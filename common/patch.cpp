/* Copyright 2000 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */


#include <unistd.h>
#include <string.h>

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>


#include "nsmtracker.h"
#include "visual_proc.h"
#include "list_proc.h"
#include "vector_proc.h"
#include "instruments_proc.h"
#include "gfx_wtrackheaders_proc.h"
#include "fxlines_proc.h"
#include "player_proc.h"
#include "player_pause_proc.h"
#include "scheduler_proc.h"

#include "undo.h"
#include "undo_tracks_proc.h"
#include "undo_patchlist_proc.h"
#include "undo_audio_patch_addremove_proc.h"
#include "windows_proc.h"
#include "notes_proc.h"
#include "../api/api_common_proc.h"
#include "../midi/midi_i_plugin_proc.h"
#include "../midi/midi_i_input_proc.h"
#include "../audio/audio_instrument_proc.h"

//#include "../mixergui/undo_chip_addremove_proc.h"
#include "../mixergui/undo_mixer_connections_proc.h"

#include "../audio/SoundPlugin.h"
#include "../audio/SoundPlugin_proc.h"
#include "../audio/SoundPluginRegistry_proc.h"
#include "../audio/Mixer_proc.h"
#include "../Qt/Qt_instruments_proc.h"

#include "patch_proc.h"


// A Major cleanup is needed for the patch/instrument system.
// (introduction of id numbers helped though)

struct Patch *g_currpatch=NULL;

//const symbol_t *g_raw_midi_message_port_name = NULL;

#define NUM_LINKED_NOTES (32*1024)
static linked_note_t *g_unused_linked_notes = NULL;


static vector_t g_unused_patches; // <-- patches are never deleted, but they are removed from the instruments. In order for the gc to find the removed patches (when stored some place where the gc can't find them, for instance in a C++ object), we put them here. (There shouldn't be any situation where this might happen, but we do it anyway, just in case, because GC bugs are so hard to find.)

// Called when loading new song
static void PATCH_clean_unused_patches(void){
  VECTOR_clean(&g_unused_patches);
}

void PATCH_remove_from_instrument(struct Patch *patch){
  //R_ASSERT(patch->patchdata == NULL); (not true for MIDI)
  VECTOR_remove(&patch->instrument->patches, patch);
  VECTOR_push_back(&g_unused_patches, patch);
}

static void PATCH_add_to_instrument(struct Patch *patch){
  if (VECTOR_is_in_vector(&g_unused_patches, patch))
    VECTOR_remove(&g_unused_patches, patch);
  VECTOR_push_back(&patch->instrument->patches, patch);
}

static vector_t *get_all_patches(void){
  vector_t *v=(vector_t*)talloc(sizeof(vector_t));
  struct Instruments *instrument = get_all_instruments();
  while(instrument!=NULL){
    VECTOR_append(v,&instrument->patches);
    instrument = NextInstrument(instrument);
  }
  return v;
}

int64_t PATCH_get_new_id(void){
  static int64_t id = 0;
  id++;
  
  // When we load a song, the songs also contains patch id number, so we need to check that the id is not already used.
  vector_t *v=get_all_patches();
  int i;
  for(i=0;i<v->num_elements;i++){
    struct Patch *patch=(struct Patch*)v->elements[i];
    if(patch->id>=id)
      id=patch->id+1;
  }

  return id;
}

struct Patch *PATCH_get_from_id(int64_t id){
  vector_t *v=get_all_patches();
  int i;
  for(i=0;i<v->num_elements;i++){
    struct Patch *patch=(struct Patch*)v->elements[i];
    if(patch->id==id)
      return patch;
  }
  return NULL;
}

void handle_fx_when_theres_a_new_patch_for_track(struct Tracks *track, struct Patch *old_patch, struct Patch *new_patch){
  R_ASSERT(PLAYER_current_thread_has_lock());
  
  if(old_patch==NULL || new_patch==NULL) {
    VECTOR_clean(&track->fxs);
    return;
  }

  if (old_patch->instrument != new_patch->instrument) {
    VECTOR_clean(&track->fxs);
    return;
  }
  
  old_patch->instrument->handle_fx_when_theres_a_new_patch_for_track(track,old_patch,new_patch);
}


void PATCH_init_voices(struct Patch *patch){
  patch->voices[0].is_on=true;

  patch->voices[1].transpose=12;
  patch->voices[2].transpose=19;
  patch->voices[3].transpose=24;
  patch->voices[4].transpose=-12;
  patch->voices[5].transpose=-24;
  patch->voices[6].transpose=0.25;

  patch->voices[0].time_format = TIME_IN_MS;
  patch->voices[1].time_format = TIME_IN_MS;
  patch->voices[2].time_format = TIME_IN_MS;
  patch->voices[3].time_format = TIME_IN_MS;
  patch->voices[4].time_format = TIME_IN_MS;
  patch->voices[5].time_format = TIME_IN_MS;
  patch->voices[6].time_format = TIME_IN_MS;
  
  patch->voices[0].chance = 256;
  patch->voices[1].chance = 256;
  patch->voices[2].chance = 256;
  patch->voices[3].chance = 256;
  patch->voices[4].chance = 256;
  patch->voices[5].chance = 256;
  patch->voices[6].chance = 256;
}

struct Patch *PATCH_alloc(void){
  struct Patch *patch = (struct Patch*)talloc(sizeof(struct Patch));

  // lots of atomic data
  patch->voices = (struct PatchVoice*)talloc_atomic_clean(sizeof(struct PatchVoice) * NUM_PATCH_VOICES);
  
  return patch;
}

void PATCH_set_name(struct Patch *patch, const char *name){
  patch->name = name==NULL ? "" : talloc_strdup(name);
  
  const char *new_name = talloc_format("Event connection from %s", patch->name);
  
  if (patch->midi_learn_port_name == NULL)
    patch->midi_learn_port_name = get_symbol(new_name);
  else
    set_symbol_name(patch->midi_learn_port_name, new_name);
}

static struct Patch *create_new_patch(const char *name){
  struct Patch *patch = PATCH_alloc();
  patch->id = PATCH_get_new_id();
  patch->forward_events = true;

  PATCH_set_name(patch, name);
  
  patch->colornum = GFX_MakeRandomCustomColor(-1);

  PATCH_init_voices(patch);

  return patch;
}

static hash_t *PATCHVOICE_get_state(struct PatchVoice voice){
  hash_t *state = HASH_create(6);
  
  HASH_put_int(state, "is_on", voice.is_on ? 1 : 0);
  HASH_put_float(state, "transpose", voice.transpose);
  HASH_put_float(state, "volume", voice.volume);
  HASH_put_float(state, "start", voice.start);
  HASH_put_float(state, "length", voice.length);
  HASH_put_int(state, "time_format", voice.time_format);
  
  return state;
}

static void apply_patchvoice_state(struct PatchVoice *voice, hash_t *state){
  voice->is_on = HASH_get_int(state, "is_on") == 1 ? true : false;
  voice->transpose = HASH_get_float(state, "transpose");
  voice->volume = HASH_get_float(state, "volume");
  voice->start = HASH_get_float(state, "start");
  voice->length = HASH_get_float(state, "length");
  voice->time_format = (TimeFormat)HASH_get_int(state, "time_format");
}
                                                
hash_t *PATCH_get_state(struct Patch *patch){
  hash_t *state = HASH_create(5);

  HASH_put_int(state, "___radium_patch_state_v3", 1);

  HASH_put_int(state, "forward_events", patch->forward_events ? 1 : 0);

  int i;
  for (i=0;i<NUM_PATCH_VOICES; i++)
    HASH_put_hash_at(state, "patchvoice", i, PATCHVOICE_get_state(patch->voices[i]));

  if (patch->instrument==get_audio_instrument()) {
    SoundPlugin *plugin = (SoundPlugin*)patch->patchdata;
    R_ASSERT(plugin!=NULL);
    if (plugin!=NULL)
      HASH_put_hash(state, "audio", PLUGIN_get_state(plugin));
  }

  // We add some more info, for future compatibility. We're not currently using any of these, but perhaps it is used later.
  HASH_put_int(state, "id", patch->id);
  HASH_put_int(state, "name_is_edited", patch->name_is_edited ? 1 : 0);
  HASH_put_chars(state, "name", patch->name);
  HASH_put_int(state, "colornum", patch->colornum);
  HASH_put_chars(state, "instrument", patch->instrument==get_audio_instrument() ? "audio" : "MIDI");
  
  return state;
}

hash_t *PATCHES_get_state(vector_t *patches, bool put_in_array){
  if (patches==NULL)
    patches = &get_audio_instrument()->patches;

  hash_t *patches_state = HASH_create(patches->num_elements);
  for(int i = 0 ; i < patches->num_elements ; i++){
    struct Patch *patch = (struct Patch*)patches->elements[i];
    hash_t *patch_state = PATCH_get_state(patch);
    if (put_in_array)
      HASH_put_hash_at(patches_state, "patch", i, patch_state);
    else
      HASH_put_hash(patches_state, talloc_format("%" PRId64, patch->id), patch_state);
  }

  if (!put_in_array)
    HASH_put_int(patches_state, "num_patches", patches->num_elements);
  
  return patches_state;
}

static void apply_patch_state(struct Patch *patch, hash_t *state){
  R_ASSERT(state!=NULL && HASH_has_key(state, "___radium_patch_state_v3"));
  patch->forward_events = HASH_get_int(state, "forward_events")==1 ? true : false;
  int i;
  for (i=0;i<HASH_get_array_size(state); i++)
    apply_patchvoice_state(&patch->voices[i], HASH_get_hash_at(state, "patchvoice", i));

  if (HASH_has_key(state, "name"))
    patch->name = HASH_get_chars(state, "name");
}


// Only called from PATCH_create_audio and undo create patch.
//
// x and y are ignored if audio_state!=NULL (since audio_state has its own "x" and "y")
bool PATCH_make_active_audio(struct Patch *patch, const char *type_name, const char *plugin_name, hash_t *state, float x, float y) {
  R_ASSERT_RETURN_IF_FALSE2(patch->instrument==get_audio_instrument(),false);

  printf("PATCH_make_active_audio called\n");

  if (VECTOR_is_in_vector(&patch->instrument->patches,patch)){
    RError("Patch %s is already active",patch->name);
    return true;
  }

  hash_t *audio_state = state;
  
  if (state!=NULL && HASH_has_key(state, "___radium_patch_state_v3")){
    audio_state = HASH_get_hash(state, "audio");
    apply_patch_state(patch, state);
  }
  
  if (AUDIO_InitPatch2(patch, type_name, plugin_name, audio_state, false, x, y)==false)
    return false;

  ADD_UNDO(Audio_Patch_Add_CurrPos(patch));

  PATCH_add_to_instrument(patch);

  return true;
}

void PATCH_init_audio_when_loading_song(struct Patch *patch, hash_t *state) {
  AUDIO_InitPatch2(patch, NULL, NULL, state, true, 0, 0);
}

// Either type_name and plugin_name is NULL, or state==NULL
struct Patch *PATCH_create_audio(const char *type_name, const char *plugin_name, const char *name, hash_t *state, float x, float y) {
  printf("PATCH_create_audio called\n");
  
  struct Patch *patch = create_new_patch(name);

  patch->instrument=get_audio_instrument();

  if (PATCH_make_active_audio(patch, type_name, plugin_name, state, x, y)==false)
    return NULL;
  
  return patch;
}

struct Patch *PATCH_create_midi(const char *name){
  struct Patch *patch = create_new_patch(name);
  
  MIDI_InitPatch(patch);

  PATCH_add_to_instrument(patch);

  return patch;
}


static QList<QString> get_plugin_effect_names(SoundPlugin *plugin){
  QList<QString> ret;
  for(int i=0;i<plugin->type->num_effects;i++)
    ret.push_back(plugin->type->get_effect_name(plugin, i));
  return ret;
}

void PATCH_handle_fxs_when_fx_names_have_changed(struct Patch *patch){
  R_ASSERT(Undo_Is_Open() || Undo_Is_Currently_Undoing() || Undo_Is_Currently_Ignoring());

  if(patch->instrument != get_audio_instrument())
    return;

  SoundPlugin *plugin = (SoundPlugin*)patch->patchdata;
  QList<QString> effect_names = get_plugin_effect_names(plugin);
  
  struct Tracker_Windows *window = root->song->tracker_windows;
  struct WBlocks *wblock = window->wblocks;
    
  while(wblock!=NULL){
    struct WTracks *wtrack = wblock->wtracks;
    while(wtrack!=NULL){
      bool has_added_undo = false;
      
      struct Tracks *track = wtrack->track;
      
      VECTOR_FOR_EACH(struct FXs *, fxs, &track->fxs){
        struct FX *fx = fxs->fx;
                   
        if (fx->patch==patch){

          int index = effect_names.indexOf(fx->name);
          
          if (index==-1) {
            
            if (has_added_undo==false){
              ADD_UNDO(Track(window,wblock,wtrack,wblock->curr_realline));
              has_added_undo = true;
            }
            
            PLAYER_lock();{
              VECTOR_remove(&track->fxs, fxs);
            }PLAYER_unlock();

            window->must_redraw = true;
            
          } else if (index != fx->effect_num) {
            if (has_added_undo==false){
              ADD_UNDO(Track(window,wblock,wtrack,wblock->curr_realline));
              has_added_undo = true;
            }

            PLAYER_lock();{            
              fx->effect_num = index;
            }PLAYER_unlock();
            
          }
          
        }
        
      }END_VECTOR_FOR_EACH;
      
      wtrack = NextWTrack(wtrack);
    }
    wblock = NextWBlock(wblock);
  }    
  
}


void PATCH_replace_patch_in_song(struct Patch *old_patch, struct Patch *new_patch){
  R_ASSERT(Undo_Is_Open() || Undo_Is_Currently_Undoing() || Undo_Is_Currently_Ignoring());

  struct Tracker_Windows *window = root->song->tracker_windows;
  struct WBlocks *wblock = window->wblocks;

  bool has_paused = false;
  
  while(wblock!=NULL){
    struct WTracks *wtrack = wblock->wtracks;
    while(wtrack!=NULL){
      bool has_added_undo = false;
      
      struct Tracks *track = wtrack->track;
      if(track->patch==old_patch){

        if (!has_paused){
          PC_Pause();
          has_paused = true;
        }
        
        ADD_UNDO(Track(window,wblock,wtrack,wblock->curr_realline));

        PLAYER_lock();{
          handle_fx_when_theres_a_new_patch_for_track(track,track->patch,new_patch);
          track->patch = new_patch;
        }PLAYER_unlock();
                  
      } else if (new_patch == NULL){

      again:
        VECTOR_FOR_EACH(struct FXs *, fxs, &track->fxs){          
          if (fxs->fx->patch==old_patch){

            if (has_added_undo==false){
              ADD_UNDO(Track(window,wblock,wtrack,wblock->curr_realline));
              has_added_undo=true;
            }
            
            PLAYER_lock();{

              VECTOR_remove(&track->fxs, fxs);
              
            }PLAYER_unlock();

            goto again;

          }
        }END_VECTOR_FOR_EACH;
        
      }
      
      wtrack = NextWTrack(wtrack);
    }
    wblock = NextWBlock(wblock);
  }

  if (has_paused)
    PC_StopPause(window);
}

static void remove_patch_from_song(struct Patch *patch){
  PATCH_replace_patch_in_song(patch, NULL);
}


static void make_inactive(struct Patch *patch, bool force_removal){

  R_ASSERT(Undo_Is_Open() || Undo_Is_Currently_Undoing() || Undo_Is_Currently_Ignoring());
  
  if (!VECTOR_is_in_vector(&patch->instrument->patches,patch)){
    RError("Patch %s is already inactive",patch->name);
    return;
  }

  if(patch->instrument!=get_audio_instrument()){
    GFX_Message(NULL, "Not possible to delete MIDI instrument");
    return;
  }

  if(force_removal==false && AUDIO_is_permanent_patch(patch)==true){
    GFX_Message(NULL,"Can not be deleted");
    return;
  }

  remove_patch_from_song(patch);

  hash_t *audio_patch_state = AUDIO_get_audio_patch_state(patch); // The state is unavailable after calling remove_patch().

  patch->instrument->remove_patchdata(patch);
  
  ADD_UNDO(Audio_Patch_Remove_CurrPos(patch, audio_patch_state)); // Must be called last, if not the undo/redo order will be wrong.

  PATCH_stop_all_notes(patch);

  PATCH_remove_from_instrument(patch);
}

void PATCH_make_inactive(struct Patch *patch){
  make_inactive(patch, false);
}

// Only used when cleaning mixer
void PATCH_force_make_inactive(struct Patch *patch){
  make_inactive(patch, true);
}
  

static bool has_recursive_event_connection(struct Patch *patch, struct Patch *start_patch){
  if(start_patch==patch)
    return true;

  int num_event_receivers = patch->num_event_receivers;
  int i;
  for(i=0; i<num_event_receivers; i++)
    if(has_recursive_event_connection(patch->event_receivers[i], start_patch)==true)
      return true;

  return false;
}

// Returns false if connection couldn't be made.
// MUST ONLY BE CALLED FROM mixergui/QM_chip.cpp:econnect (because PATCH_add_event_receiver must be called at the same time as SP_add_link)
bool PATCH_add_event_receiver(struct Patch *source, struct Patch *destination){
  int num_event_receivers = source->num_event_receivers;

  if(num_event_receivers==MAX_NUM_EVENT_RECEIVERS) {
    printf("No more free event receiver slots. (wow)\n");
    return false;
  }

  int i;
  for(i=0; i<num_event_receivers; i++)
    if(source->event_receivers[i]==destination){
      printf("Already connectied\n");
      return false;
    }

  if(has_recursive_event_connection(destination, source)==true) {
    GFX_Message(NULL, "Recursive event connections are not supported.\n(Send a feature request if you need it!)");
    printf("Recursive attempt\n");
    return false;
  }

  PLAYER_lock();{
    source->event_receivers[num_event_receivers] = destination;
    source->num_event_receivers = num_event_receivers+1;
  }PLAYER_unlock();

  return true;
}

// MUST ONLY BE CALLED FROM mixergui/QM_chip.cpp:PATCH_remove_event_receiver (because PATCH_add_event_receiver must be called at the same time as SP_remove_link)
void PATCH_remove_event_receiver(struct Patch *source, struct Patch *destination){
  int i;
  int num_event_receivers = source->num_event_receivers;

  PLAYER_lock();{
    for(i=0; i<num_event_receivers; i++){
      if(source->event_receivers[i]==destination){
        source->event_receivers[i] = source->event_receivers[num_event_receivers-1];
        source->num_event_receivers = num_event_receivers-1;
        //printf("Removed event receiver\n");
        break;
      }
    }
  }PLAYER_unlock();
}

void PATCH_remove_all_event_receivers(struct Patch *patch){
  int num_event_receivers = patch->num_event_receivers;

  PLAYER_lock();{
    int i;
    for(i=0; i<num_event_receivers; i++)
      patch->event_receivers[i] = NULL;

    patch->num_event_receivers = 0;
  }PLAYER_unlock();
}



void PATCH_call_very_often(void){
  struct Instruments *instrument = get_all_instruments();

  while(instrument!=NULL){

    VECTOR_FOR_EACH(struct Patch *, patch, &instrument->patches){
      if (ATOMIC_COMPARE_AND_SET_BOOL(patch->widget_needs_to_be_updated, true, false))
        GFX_update_instrument_widget(patch);
    }END_VECTOR_FOR_EACH;

    instrument = NextInstrument(instrument);
  }

}

static void add_linked_note(linked_note_t **root, linked_note_t *note);
static int num_linked_notes(linked_note_t *root);
static void reset_unused_linked_notes(void);
  
// Called when loading new song
void PATCH_reset(void){
  PATCH_clean_unused_patches();
  reset_unused_linked_notes();
}

void PATCH_init(void){
  //g_raw_midi_message_port_name = get_symbol("Radium: Event connection");
  //MUTEX_INITIALIZE();

  PATCH_reset();
}



////////////////////////////////////
// Play note

static void reset_unused_linked_notes(void){
  size_t size = sizeof(linked_note_t) * NUM_LINKED_NOTES;
  static linked_note_t *notes = NULL;

#if !defined(RELEASE)
  if (notes != NULL)
    R_ASSERT(num_linked_notes(g_unused_linked_notes)==NUM_LINKED_NOTES);
#endif

  if (notes==NULL)
    notes = (linked_note_t*)V_calloc(1, size);
  else
    memset(notes, 0, size); // Not really necessary

  g_unused_linked_notes = NULL;
  
  for(int i = NUM_LINKED_NOTES-1 ; i >= 0 ; i--)
    add_linked_note(&g_unused_linked_notes, &notes[i]);
}

static int num_linked_notes(linked_note_t *root){
  int ret = 0;
  for(linked_note_t *note = root ; note!=NULL ; note=note->next)
    ret++;
  return ret;
}

static void add_linked_note(linked_note_t **rootp, linked_note_t *note){
  linked_note_t *root = *rootp;
  
  if (root==NULL) {
    
    note->prev = NULL;
    note->next = NULL;
    
  } else {

    note->prev = NULL;
    note->next = root;
    root->prev = note;
    
  }

  *rootp = note;
}

static void remove_linked_note(linked_note_t **rootp, linked_note_t *note){

  if (note->prev == NULL) {
    *rootp = note->next;
  } else
    note->prev->next = note->next;

  if (note->next != NULL)
    note->next->prev = note->prev;
}

static linked_note_t *find_linked_note(linked_note_t *root, int64_t id, const struct SeqBlock *seqblock){
  for(linked_note_t *note = root ; note!=NULL ; note=note->next)
    if(is_note(note->note, id, seqblock))
      return note;

  return NULL;
}

static bool Patch_is_voice_playing_questionmark(struct Patch *patch, int64_t voice_id, const struct SeqBlock *seqblock){
  R_ASSERT_NON_RELEASE(PLAYER_current_thread_has_lock());

  if (find_linked_note(patch->playing_voices, voice_id, seqblock) != NULL)
    return true;

  return false;
}

bool Patch_addPlayingVoice(linked_note_t **rootp, const note_t note, struct SeqTrack *seqtrack){
  R_ASSERT_RETURN_IF_FALSE2(seqtrack!=NULL, false);

#if 0
  printf("Adding note with id %d\n",(int)note_id);
  if(note_id==52428)
    abort();
#endif

  R_ASSERT_NON_RELEASE(PLAYER_current_thread_has_lock());

  R_ASSERT_RETURN_IF_FALSE2(seqtrack!=NULL, false);
  
  linked_note_t *linked_note = g_unused_linked_notes;
  
  if (linked_note==NULL){
    RT_message("Error. Reached max number of voices it's possible to play.\n");
    return false;
  }
  
  remove_linked_note(&g_unused_linked_notes, linked_note);

  linked_note->note = note;
  linked_note->seqtrack = seqtrack;
  
  add_linked_note(rootp, linked_note);

  return true;
}


void Patch_removePlayingVoice(linked_note_t **rootp, int64_t note_id, struct SeqTrack *seqtrack, const struct SeqBlock *seqblock){
  R_ASSERT_RETURN_IF_FALSE(seqtrack!=NULL);
  
  R_ASSERT_NON_RELEASE(PLAYER_current_thread_has_lock());
 
  linked_note_t *linked_note = find_linked_note(*rootp, note_id, seqblock);

#if !defined(RELEASE)
  if (linked_note==NULL && is_playing())
    printf("Warning. Unable to find voice with note_id %d when removing playing note. Num playing: %d\n",(int)note_id,num_linked_notes(*rootp)); // think there are legitimate situations where this can happen
#endif

  if (linked_note!=NULL){
    R_ASSERT_NON_RELEASE(linked_note->seqtrack==seqtrack); // Think there are legitimate situations where this can happen
    remove_linked_note(rootp, linked_note);
    add_linked_note(&g_unused_linked_notes, linked_note);
    linked_note->seqtrack = NULL;
  }
}

static bool Patch_addPlayingNote(struct Patch *patch, const note_t note, struct SeqTrack *seqtrack){
  R_ASSERT_RETURN_IF_FALSE2(seqtrack!=NULL, false);
  
  linked_note_t *linked_note = g_unused_linked_notes;

  if (linked_note==NULL){
    RT_message("Error. Reached max number of notes it's possible to play.\n");
    return false;
  }

  remove_linked_note(&g_unused_linked_notes, linked_note);

  linked_note->note = note;
  linked_note->seqtrack = seqtrack;
  
  add_linked_note(&patch->playing_notes, linked_note);

  return true;
}

static void Patch_removePlayingNote(struct Patch *patch, int64_t note_id, struct SeqTrack *seqtrack, const struct SeqBlock *seqblock){
  R_ASSERT_RETURN_IF_FALSE(seqtrack!=NULL);
  
  linked_note_t *linked_note = find_linked_note(patch->playing_notes, note_id, seqblock);

  
#if !defined(RELEASE)
  if (linked_note==NULL && is_playing())
    printf("Warning. Unable to find note with note_id %d when removing playing note\n",(int)note_id); // think there are legitimate situations where this can happen
#endif
  
  if (linked_note!=NULL){
    //R_ASSERT(linked_note->seqtrack==seqtrack); // There are legitimate situations where this can happen
    remove_linked_note(&patch->playing_notes, linked_note);
    add_linked_note(&g_unused_linked_notes, linked_note);
    linked_note->seqtrack = NULL;
  }
}

static float get_voice_velocity(struct PatchVoice *voice){
  if(voice->volume<=35)
    return scale(voice->volume,-35,35,0,2);
  else
    return scale(voice->volume,35,70,2,7);
}

void RT_PATCH_send_play_note_to_receivers(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  int i;

  if (patch->instrument==get_audio_instrument())
    RT_PLUGIN_touch((struct SoundPlugin*)patch->patchdata);
             
  for(i = 0; i<patch->num_event_receivers; i++) {
    struct Patch *receiver = patch->event_receivers[i];
    R_ASSERT_RETURN_IF_FALSE(receiver!=patch); // unnecessary. We detect recursions when creating connections. (Not just once (which should have been enough) but both here and in SoundProducer.cpp)
    RT_PATCH_play_note(seqtrack, receiver, note, time);
  }
}

static void RT_play_voice(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){

  if(note.pitch < 1.0 || note.pitch > 127)
    return;

  if (!Patch_addPlayingVoice(&patch->playing_voices, note, seqtrack))
    return;

  patch->playnote(seqtrack, patch,note,time);

  ATOMIC_SET_RELAXED(patch->visual_note_intencity, MAX_NOTE_INTENCITY);

  if(patch->forward_events)
    RT_PATCH_send_play_note_to_receivers(seqtrack, patch, note, time);
}

static int64_t RT_scheduled_play_voice(struct SeqTrack *seqtrack, int64_t time, union SuperType *args){
  struct Patch *patch = (struct Patch*)args[0].pointer;

  const note_t note = create_note_from_args(&args[1]);

  //printf("playing scheduled play note: %d. time: %d, velocity: %d\n",notenum,(int)time,velocity);
  //return;

  //printf("___RT_scheduled_play_voice. time: %d\n", (int)time);

  RT_play_voice(seqtrack,
                patch,
                note,
                time);

  return DONT_RESCHEDULE;
}


static int64_t RT_scheduled_stop_voice(struct SeqTrack *seqtrack, int64_t time_into_the_future, union SuperType *args);

static int rnd(int max){
  return rand() % max;
}

int64_t RT_PATCH_play_note(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  //printf("\n\nRT_PATCH_PLAY_NOTE. ___Starting note %f, time: %d, id: %d. block_reltempo: %f\n\n",note.pitch,(int)time,(int)note.id,note.block_reltempo);

  if (!Patch_addPlayingNote(patch, note, seqtrack))
    return note.id;

  float sample_rate = MIXER_get_sample_rate();

  union SuperType args[7];      
  args[0].pointer = patch;
  put_note_into_args(&args[1], note);

  int i;
  for(i=0;i<NUM_PATCH_VOICES;i++){
    struct PatchVoice *voice = &patch->voices[i];

    if(voice->is_on==true && (voice->chance==256 || voice->chance > rnd(256))){

      float voice_notenum = note.pitch + voice->transpose;
      float voice_velocity = note.velocity * get_voice_velocity(voice);
      int64_t voice_id = note.id + i;

      args[1].float_num = voice_notenum;
      args[2].int_num = voice_id;
      args[3].float_num = voice_velocity;

      // voice ON
      //printf("___RT_PATCH_play_note. time: %d (%d)\n", (int)time, (int)(time + voice->start*sample_rate/1000));

      SCHEDULER_add_event(seqtrack, time + voice->start*sample_rate/1000, RT_scheduled_play_voice, &args[0], 7, SCHEDULER_NOTE_ON_PRIORITY);

      // voice OFF
      if(voice->length>0.001) // The voice decides when to stop by itself.
        SCHEDULER_add_event(seqtrack, time + (voice->start+voice->length)*sample_rate/1000, RT_scheduled_stop_voice, &args[0], 7, SCHEDULER_NOTE_OFF_PRIORITY);
    }
  }

  return note.id;
}

//extern const char *NotesTexts3[131];

int64_t PATCH_play_note(struct Patch *patch, const note_t note){
  //printf("** playing note %s\n",NotesTexts3[notenum]);
  int64_t ret;
  
  PLAYER_lock();{
    ret = RT_PATCH_play_note(RT_get_curr_seqtrack(), patch, note, RT_get_curr_seqtrack()->start_time);
  }PLAYER_unlock();

  return ret;
}


////////////////////////////////////
// Stop note

void RT_PATCH_send_stop_note_to_receivers(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  int i;

  if (patch->instrument==get_audio_instrument())
    RT_PLUGIN_touch((struct SoundPlugin*)patch->patchdata);
    
  for(i = 0; i<patch->num_event_receivers; i++) {
    struct Patch *receiver = patch->event_receivers[i];
    RT_PATCH_stop_note(seqtrack, receiver, note, time);
  }
}

static void RT_stop_voice(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  if(note.pitch < 0.0 || note.pitch>127)
    return;

#if 0
  if(track!=NULL && track->volumeonoff)
    velocity = PATCH_radiumvelocity_to_patchvelocity(patch,track->volume*velocity)/MAXTRACKVOL;
  else
    velocity = PATCH_radiumvelocity_to_patchvelocity(patch,velocity);
#endif

  bool voice_is_playing = Patch_is_voice_playing_questionmark(patch, note.id, note.seqblock);
  //printf("voice_is_playing: %d\n",voice_is_playing);
  
  if (voice_is_playing) {
    Patch_removePlayingVoice(&patch->playing_voices, note.id, seqtrack, note.seqblock);
    patch->stopnote(seqtrack, patch, note, time);
  }
  
  if(patch->forward_events)
    RT_PATCH_send_stop_note_to_receivers(seqtrack, patch, note, time);
}

static int64_t RT_scheduled_stop_voice(struct SeqTrack *seqtrack, int64_t time, union SuperType *args){
  struct Patch *patch = (struct Patch*)args[0].pointer;

  const note_t note = create_note_from_args(&args[1]);
  //printf("       RT_scheduled_stop_voice called for note %d\n", (int)note.id);

  //printf("stopping scheduled play note: %d. time: %d, velocity: %d\n",notenum,(int)time,velocity);
  //return;

  RT_stop_voice(seqtrack, patch, note, time);

  return DONT_RESCHEDULE;
}

void RT_PATCH_stop_note(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  //printf("\n\nRT_PATCH_STOP_NOTE. ___Stopping note %f, time: %d, id: %d\n\n",note.pitch,(int)time,(int)note.id);

  Patch_removePlayingNote(patch, note.id, seqtrack, note.seqblock);

  float sample_rate = MIXER_get_sample_rate();

  union SuperType args[7];
  args[0].pointer = patch;
  put_note_into_args(&args[1], note);

  int i;
  for(i=0;i<NUM_PATCH_VOICES;i++){
    struct PatchVoice *voice = &patch->voices[i];

    if(voice->is_on==true){

      if(voice->length<=0.001) { // i.e. this voice does not use a stopping time defined in the editor.

        float voice_notenum = note.pitch + voice->transpose;
        int64_t voice_id = note.id + i;

        //printf("RT_PATCH_stop_note. voice_id: %d. voice_is_playing: %d\n",(int)voice_id,(int)voice_is_playing);
        
        args[1].float_num = voice_notenum;
        args[2].int_num = voice_id;

        SCHEDULER_add_event(seqtrack, time + voice->start*sample_rate/1000, RT_scheduled_stop_voice, &args[0], 7, SCHEDULER_NOTE_OFF_PRIORITY);
      }
    }
  }
}

void PATCH_stop_note(struct Patch *patch, const note_t note){
  //printf("** stopping note %s\n\n",NotesTexts3[notenum]);
  PLAYER_lock();{
    RT_PATCH_stop_note(RT_get_curr_seqtrack(), patch,note,RT_get_curr_seqtrack()->start_time);
  }PLAYER_unlock();
}



////////////////////////////////////
// Change velocity

void RT_PATCH_send_change_velocity_to_receivers(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  //printf("\n\nRT_PATCH_VELOCITY. ___velocity for note %f, time: %d, id: %d (vel: %f)\n\n",notenum,(int)time,(int)note_id,velocity);

  if (patch->instrument==get_audio_instrument())
    RT_PLUGIN_touch((struct SoundPlugin*)patch->patchdata);
    
  int i;

  for(i = 0; i<patch->num_event_receivers; i++) {
    struct Patch *receiver = patch->event_receivers[i];
    RT_PATCH_change_velocity(seqtrack, receiver, note, time);
  }
}

static void RT_change_voice_velocity(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  if(note.pitch < 1 || note.pitch>127)
    return;

#if 0
  if(track!=NULL && track->volumeonoff)
    velocity = PATCH_radiumvelocity_to_patchvelocity(patch,track->volume*velocity)/MAXTRACKVOL;
  else
    velocity = PATCH_radiumvelocity_to_patchvelocity(patch,velocity);
#endif

  if (Patch_is_voice_playing_questionmark(patch, note.id, note.seqblock))
    patch->changevelocity(seqtrack,patch,note,time);

  if(patch->forward_events)
    RT_PATCH_send_change_velocity_to_receivers(seqtrack, patch, note, time);
}

static int64_t RT_scheduled_change_voice_velocity(struct SeqTrack *seqtrack, int64_t time, union SuperType *args){
  struct Patch *patch = (struct Patch*)args[0].pointer;

  const note_t note = create_note_from_args(&args[1]);

  //printf("stopping scheduled play note: %d. time: %d, velocity: %d\n",notenum,(int)time,velocity);
  //return;

  RT_change_voice_velocity(seqtrack, patch,note,time);

  return DONT_RESCHEDULE;
}

void RT_PATCH_change_velocity(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  //printf("vel: %d\n",velocity);

  float sample_rate = MIXER_get_sample_rate();

  union SuperType args[7];
  args[0].pointer = patch;
  put_note_into_args(&args[1], note);

  int i;
  for(i=0;i<NUM_PATCH_VOICES;i++){
    struct PatchVoice *voice = &patch->voices[i];

    if(voice->is_on==true){

      float voice_notenum = note.pitch + voice->transpose;
      int64_t voice_id = note.id + i;
      float voice_velocity = note.velocity * get_voice_velocity(voice);

      args[1].float_num = voice_notenum;
      args[2].int_num = voice_id;
      args[3].float_num = voice_velocity;
      
      SCHEDULER_add_event(seqtrack, time + voice->start*sample_rate/1000, RT_scheduled_change_voice_velocity, &args[0], 7, SCHEDULER_VELOCITY_PRIORITY);
    }
  }
}

void PATCH_change_velocity(struct Patch *patch, const note_t note){
  PLAYER_lock();{
    RT_PATCH_change_velocity(RT_get_curr_seqtrack(), patch,note,RT_get_curr_seqtrack()->start_time);
  }PLAYER_unlock();
}



////////////////////////////////////
// Change pitch

void RT_PATCH_send_change_pitch_to_receivers(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  int i;

  if (patch->instrument==get_audio_instrument())
    RT_PLUGIN_touch((struct SoundPlugin*)patch->patchdata);
  
  for(i = 0; i<patch->num_event_receivers; i++) {
    struct Patch *receiver = patch->event_receivers[i];
    RT_PATCH_change_pitch(seqtrack, receiver, note, time);
  }
}

static void RT_change_voice_pitch(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  if(note.pitch < 1 || note.pitch>127)
    return;

  //printf("Calling patch->changeptitch %d %f\n",notenum,pitch);
  if (Patch_is_voice_playing_questionmark(patch, note.id, note.seqblock))
    patch->changepitch(seqtrack, patch,note,time);

  if(patch->forward_events)
    RT_PATCH_send_change_pitch_to_receivers(seqtrack, patch, note, time);
}

static int64_t RT_scheduled_change_voice_pitch(struct SeqTrack *seqtrack, int64_t time, union SuperType *args){
  struct Patch *patch = (struct Patch*)args[0].pointer;

  const note_t note = create_note_from_args(&args[1]);

  RT_change_voice_pitch(seqtrack,patch,note,time);

  return DONT_RESCHEDULE;
}

void RT_PATCH_change_pitch(struct SeqTrack *seqtrack, struct Patch *patch, const note_t note, STime time){
  //printf("vel: %d\n",pitch);

  float sample_rate = MIXER_get_sample_rate();

  union SuperType args[7];
  args[0].pointer = patch;
  put_note_into_args(&args[1], note);
     

  int i;
  for(i=0;i<NUM_PATCH_VOICES;i++){
    struct PatchVoice *voice = &patch->voices[i];

    if(voice->is_on==true){

      float voice_notenum = note.pitch + voice->transpose;
      int64_t voice_id = note.id + i;

      args[1].float_num = voice_notenum;
      args[2].int_num = voice_id;
      
      SCHEDULER_add_event(seqtrack, time + voice->start*sample_rate/1000, RT_scheduled_change_voice_pitch, &args[0], 7, SCHEDULER_PITCH_PRIORITY);
    }
  }
}

void PATCH_change_pitch(struct Patch *patch, const note_t note){
  PLAYER_lock();{
    RT_PATCH_change_pitch(RT_get_curr_seqtrack(), patch,note,RT_get_curr_seqtrack()->start_time);
  }PLAYER_unlock();
}



////////////////////////////////////
// Raw midi messages

extern void aiai(int64_t id, const symbol_t *port_name, uint32_t msg);

void RT_PATCH_send_raw_midi_message_to_receivers(struct SeqTrack *seqtrack, struct Patch *patch, uint32_t msg, STime time){

  if (patch->instrument==get_audio_instrument())
    RT_PLUGIN_touch((struct SoundPlugin*)patch->patchdata);
  
  int i;

  for(i = 0; i<patch->num_event_receivers; i++) {
    struct Patch *receiver = patch->event_receivers[i];    
    MidiLearn::RT_maybe_use_forall(receiver->id, patch->midi_learn_port_name, msg);
    RT_PATCH_send_raw_midi_message(seqtrack, receiver, msg, time);
  }
}

static void RT_send_raw_midi_message(struct SeqTrack *seqtrack, struct Patch *patch, uint32_t msg, STime time, double block_reltempo){
  patch->sendrawmidimessage(seqtrack,patch,msg,time, block_reltempo);

  if(patch->forward_events)
    RT_PATCH_send_raw_midi_message_to_receivers(seqtrack, patch, msg, time);
}

static int64_t RT_scheduled_send_raw_midi_message(struct SeqTrack *seqtrack, int64_t time, union SuperType *args){
  struct Patch *patch = (struct Patch*)args[0].pointer;

  uint32_t msg = args[1].uint32_num;

  double block_reltempo = args[2].float_num;
  
  RT_send_raw_midi_message(seqtrack, patch, msg, time, block_reltempo);

  return DONT_RESCHEDULE;
}

void RT_PATCH_send_raw_midi_message(struct SeqTrack *seqtrack, struct Patch *patch, uint32_t msg, STime time){

  float sample_rate = MIXER_get_sample_rate();
    
  int i;
  for(i=0;i<NUM_PATCH_VOICES;i++){
    struct PatchVoice *voice = &patch->voices[i];

    if(voice->is_on==true){

      union SuperType args[3];
      
      args[0].pointer = patch;
      args[1].uint32_num = msg;
      args[2].float_num = get_seqtrack_reltempo(seqtrack); // Fix? This is not always correct.
      
      SCHEDULER_add_event(seqtrack, time + voice->start*sample_rate/1000, RT_scheduled_send_raw_midi_message, &args[0], 3, SCHEDULER_RAWMIDIMESSAGE_PRIORITY);
    }
  }
}

void PATCH_send_raw_midi_message(struct Patch *patch, uint32_t msg){
  PLAYER_lock();{
    RT_PATCH_send_raw_midi_message(RT_get_curr_seqtrack(),patch,msg,RT_get_curr_seqtrack()->start_time);
  }PLAYER_unlock();
}



////////////////////////////////////
// FX

// All FX goes through this function.

void RT_FX_treat_fx(struct SeqTrack *seqtrack, struct FX *fx,int val,STime time,int skip, FX_when when){
  struct Patch *patch = fx->patch;
  R_ASSERT_RETURN_IF_FALSE(patch!=NULL);

  //printf("RT_FX_treat_fx %d %d\n",(int)time,val);
  fx->treatFX(seqtrack,fx,val,time,skip,when,get_seqtrack_reltempo(seqtrack));
}

/*
void FX_treat_fx(struct FX *fx,int val,int skip){
  PLAYER_lock();{
    RT_FX_treat_fx(fx,val,SCHEDULE_NOW,skip, FX_single);
  }PLAYER_unlock();
}
*/


////////////////////////////////////
//

static void RT_PATCH_turn_voice_on(struct SeqTrack *seqtrack, struct Patch *patch, int voicenum){ 

  struct PatchVoice *voice = &patch->voices[voicenum];

  float voice_velocity = get_voice_velocity(voice);

  if(voice->is_on==false){

    for(linked_note_t *linked_note = patch->playing_notes ; linked_note!=NULL ; linked_note=linked_note->next) {
      
      const note_t &note = linked_note->note;

      RT_play_voice(seqtrack,
                    patch,
                    create_note_t(note.seqblock,
                                  note.id + voicenum,
                                  note.pitch + voice->transpose,
                                  voice_velocity,
                                  note.pan,
                                  note.midi_channel
                                  ),
                    seqtrack->start_time
                    );
  }    
    voice->is_on = true;
  }
}

static void RT_PATCH_turn_voice_off(struct SeqTrack *seqtrack, struct Patch *patch, int voicenum){ 

  struct PatchVoice *voice = &patch->voices[voicenum];

  if(voice->is_on==true){

    for(linked_note_t *linked_note = patch->playing_notes ; linked_note!=NULL ; linked_note=linked_note->next) {
      
      const note_t &note = linked_note->note;

      RT_stop_voice(seqtrack,
                    patch,
                    create_note_t(note.seqblock, note.id + voicenum,
                                  note.pitch + voice->transpose,
                                  note.velocity,
                                  note.pan,
                                  note.midi_channel
                                  ),
                    seqtrack->start_time
                    );
    }
    voice->is_on = false;
  }
}

void PATCH_turn_voice_on(struct Patch *patch, int voicenum){ 
  PLAYER_lock();{
    RT_PATCH_turn_voice_on(RT_get_curr_seqtrack(),patch,voicenum);
  }PLAYER_unlock();
}

void PATCH_turn_voice_off(struct Patch *patch, int voicenum){ 
  PLAYER_lock();{
    RT_PATCH_turn_voice_off(RT_get_curr_seqtrack(),patch,voicenum);
  }PLAYER_unlock();
}

void PATCH_change_voice_transpose(struct Patch *patch, int voicenum, float new_transpose){
  PLAYER_lock();{
    bool was_on = patch->voices[voicenum].is_on;

    if(was_on==true)
      RT_PATCH_turn_voice_off(RT_get_curr_seqtrack(),patch,voicenum);
    
    patch->voices[voicenum].transpose = new_transpose;
    
    if(was_on==true)
      RT_PATCH_turn_voice_on(RT_get_curr_seqtrack(),patch,voicenum);

  }PLAYER_unlock();
}

// Note: This function does not guarantee that all notes are stopped. A note can be scheduled
// to start playing after this function returns.
void PATCH_stop_all_notes(struct Patch *patch){

  printf("STOP ALL NOTES on \"%s\".\n", patch->name);

  {
    radium::PlayerLock lock;
    
    // Note that we use RT_stop_voice instead of RT_stop_note to turn off sound.
    //
    // The reason is that all sound must be stopped after returning from this function,
    // but that doesn't necessarily happen when calling RT_stop_note since RT_stop_note just
    // schedules new calls to RT_stop_voice.
    
    // 1. Clean patch->playing_voices
    //
    while(patch->playing_voices != NULL) {
      RT_stop_voice(patch->playing_voices->seqtrack,
                    patch,                    
                    patch->playing_voices->note,
                    patch->playing_voices->seqtrack->start_time
                    );
    }

    // 2. Clean patch->playing_notes
    //
    while(patch->playing_notes != NULL)
      Patch_removePlayingNote(patch, patch->playing_notes->note.id, patch->playing_notes->seqtrack, patch->playing_notes->note.seqblock);

    // 3. Do the same to the playing voices in the audio system. (may be hanging notes there due to notes started later because of latency compensation)
    //
    if (patch->instrument==get_audio_instrument())
      AUDIO_stop_all_notes(patch);

#if !defined(RELEASE)
    R_ASSERT(patch->playing_notes==NULL);
    R_ASSERT(patch->playing_notes==NULL);
#endif

  }
}

void PATCH_playNoteCurrPos(struct Tracker_Windows *window, float notenum, int64_t note_id){
	struct Tracks *track=window->wblock->wtrack->track;
	struct Patch *patch=track->patch;

	if(patch==NULL || notenum<0 || notenum>127) return;

	PATCH_play_note(patch,
                        create_note_t(NULL,
                                      note_id,
                                      notenum,
                                      TRACK_get_volume(track),
                                      TRACK_get_pan(track),
                                      0
                                      )
                        );
}


void PATCH_stopNoteCurrPos(struct Tracker_Windows *window,float notenum, int64_t note_id){
	struct Tracks *track=window->wblock->wtrack->track;
	struct Patch *patch=track->patch;

	if(patch==NULL || notenum<0 || notenum>127) return;

	PATCH_stop_note(patch,
                        create_note_t2(NULL, note_id, notenum)
                        );
}

// Must only be called if TRACK_has_peaks(track)==true.
int PATCH_get_peaks(struct Patch *patch,
                    float notenum, 
                    int ch, 
                    const struct Tracks *track, 
                    int64_t start_time, int64_t end_time, 
                    float *min_value, float *max_value
                    )
{
  int ret = 0;
  SoundPlugin *plugin=(SoundPlugin*)patch->patchdata;

  if(ch==-1) {

    if(track!=NULL && track->panonoff)
      return 2;
    else
      return plugin->type->get_peaks(plugin,
                                     0,
                                     -1,
                                     0,
                                     0,0,NULL,NULL);

  }

  float sample_rate = MIXER_get_sample_rate();

  float pan = 0.0f;

  if(track!=NULL && track->panonoff)
    pan = scale(track->pan,-MAXTRACKPAN,MAXTRACKPAN,-1.0f,1.0f);
                

  float min=0.0f;
  float max=0.0f;

  int i;

  for(i=0;i<NUM_PATCH_VOICES;i++){
    struct PatchVoice *voice = &patch->voices[i];

    // This didn't turn out very pretty.
    if(voice->is_on==true){

      float voice_notenum = notenum + (float)voice->transpose;

      if(voice_notenum > 0 && voice_notenum<128){

        int64_t voice_start_time = start_time - voice->start*sample_rate/1000;

        if(voice_start_time >= 0.0f){
          float min2;
          float max2;

          int64_t voice_end_time = end_time - voice->start*sample_rate/1000;

          if(voice->length<=0.001 || ( voice_start_time < (voice->length*sample_rate/1000))){

            ret = plugin->type->get_peaks(plugin,
                                          voice_notenum,
                                          ch,
                                          pan,
                                          voice_start_time,
                                          voice_end_time,
                                          &min2,
                                          &max2);
            
            min2 *= get_voice_velocity(voice);
            max2 *= get_voice_velocity(voice);
            
            if(min2<min)
              min=min2;
            if(max2>max)
              max=max2;
          }
        }
      }
    }
  }

  *min_value = min;
  *max_value = max;

  return ret;
}
