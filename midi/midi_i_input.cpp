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


#include <boost/version.hpp>
#if (BOOST_VERSION < 100000) || ((BOOST_VERSION / 100 % 1000) < 58)
  #error "Boost too old. Need at least 1.58.\n Quick fix: cd $HOME ; wget http://downloads.sourceforge.net/project/boost/boost/1.60.0/boost_1_60_0.tar.bz2 ; tar xvjf boost_1_60_0.tar.bz2 (that's it!)"
#endif
#include <boost/lockfree/queue.hpp>



#include "../api/api_proc.h"

#include "nsmtracker.h"

#include "../common/list_proc.h"
#include "../common/hashmap_proc.h"
#include "../common/notes_proc.h"
#include "../common/blts_proc.h"
#include "../common/OS_Ptask2Mtask_proc.h"
#include "../common/player_proc.h"
#include "../common/patch_proc.h"
#include "../common/instruments_proc.h"
#include "../common/placement_proc.h"
#include "../common/time_proc.h"
#include "../common/hashmap_proc.h"
#include "../common/undo.h"
#include "../common/undo_notes_proc.h"
#include "../common/undo_fxs_proc.h"
#include "../common/settings_proc.h"
#include "../common/OS_Player_proc.h"
#include "../common/visual_proc.h"
#include "../common/Mutex.hpp"
#include "../common/Queue.hpp"
#include "../common/Vector.hpp"

#include "../audio/Mixer_proc.h"
#include "../audio/SoundPlugin.h"
#include "../audio/SoundPlugin_proc.h"

#include "midi_i_plugin.h"
#include "midi_i_plugin_proc.h"
#include "midi_proc.h"

#include "midi_i_input_proc.h"


static DEFINE_ATOMIC(uint32_t, g_msg) = 0;

static DEFINE_ATOMIC(struct Patch *, g_through_patch) = NULL;

extern const char *NotesTexts3[131];

static radium::Mutex g_midi_learns_mutex;
static radium::Vector<MidiLearn*> g_midi_learns;

static bool msg_is_fx(const uint32_t msg){
  int cc0 = MIDI_msg_byte1_remove_channel(msg);
  if (cc0==0xb0 || cc0==0xe0)
    return true;
  else
    return false;
}

static bool msg_is_note(const uint32_t msg){
  int cc0 = MIDI_msg_byte1_remove_channel(msg);
  if (cc0 == 0x80 || cc0==0x90)
    return true;
  else
    return false;
}

static bool msg_is_note_on(const uint32_t msg){
  int cc0 = MIDI_msg_byte1_remove_channel(msg);
  int data2 = MIDI_msg_byte2(msg);
  if (cc0 == 0x90 && data2>0)
    return true;
  else
    return false;
}

static bool msg_is_note_off(const uint32_t msg){
  int      cc0     = MIDI_msg_byte1_remove_channel(msg);
  int      volume  = MIDI_msg_byte3(msg);

  if (cc0==0x80 || (cc0=0x90 && volume==0))
    return true;
  else
    return false;
}

static float get_msg_fx_value(uint32_t msg){
  int d1 = MIDI_msg_byte1(msg);
  int d2 = MIDI_msg_byte2(msg);
  int d3 = MIDI_msg_byte3(msg);

  if (d1 < 0xc0) {

    // cc
    
    return d3 / 127.0;
      
    
  } else {

    // pitch bend

      int val = d3<<7 | d2;
      //printf("     d2: %x, d3: %x, %x\n", d2,d3, d3<<7 | d2);
      
      if (val < 0x2000)
        return scale(val,
                     0, 0x2000,
                     0, 0.5
                     );
      else
        return scale(val,
                     0x2000,   0x3fff,
                     0.5,      1.0
                     );
  }
}



/*********************************************************
 *********************************************************
 **          Configuration                              **
 *********************************************************
 *********************************************************/

static bool g_record_accurately_while_playing = true;

bool MIDI_get_record_accurately(void){
  return g_record_accurately_while_playing;
}

void MIDI_set_record_accurately(bool accurately){
  SETTINGS_write_bool("record_midi_accurately", accurately);
  g_record_accurately_while_playing = accurately;
}

static bool g_record_velocity = true;

bool MIDI_get_record_velocity(void){
  return g_record_velocity;
}

void MIDI_set_record_velocity(bool doit){
  printf("doit: %d\n",doit);
  SETTINGS_write_bool("always_record_midi_velocity", doit);
  g_record_velocity = doit;
}





/*********************************************************
 *********************************************************
 **          Record MIDI                                **
 *********************************************************
 *********************************************************/


typedef struct _midi_event_t{
  time_position_t timepos;
  uint32_t msg;
  SoundPlugin *plugin;
  int effect_num;
} midi_event_t;


static radium::Mutex g_midi_event_mutex;
static radium::Vector<midi_event_t> g_recorded_midi_events;

static radium::Queue<midi_event_t, 8000> g_midi_event_queue; // 8000 is not the maximum size that can be recorded totally, but the maximum size that can be recorded until the recording queue pull thread has a chance to pick it up. In other words: 8000 should be plenty enough as long as the CPU is not too busy.

  
// Called from a MIDI input thread. Only called if playing
static void record_midi_event(const symbol_t *port_name, const uint32_t msg){

  bool is_fx = msg_is_fx(msg);
  bool is_note = msg_is_note(msg);

#if defined(RELEASE)
  if (!is_fx && !is_note)
    return;
#endif
  
  time_position_t timepos;
  
  if (MIXER_fill_in_time_position(&timepos) == true){ // <-- MIXER_fill_in_time_position returns false if the event was recorded after song end, or program is initializing, or there was an error.

    if (is_fx){
      
      radium::ScopedMutex lock(&g_midi_learns_mutex); // <- Fix. Timing can be slightly inaccurate if adding/removing midi learn while recording, since MIDI_add/remove_midi_learn obtains the player lock while holding the g_midi_learns_mutex.
    
      for (auto midi_learn : g_midi_learns) {
        
        if (midi_learn->RT_matching(port_name, msg)){

          midi_event_t midi_event;
          midi_event.timepos = timepos;
          midi_event.msg = msg;
          
          if (midi_learn->RT_get_automation_recording_data(&midi_event.plugin, &midi_event.effect_num)){

            // Send event to the pull thread
            if (!g_midi_event_queue.tryPut(midi_event))
              RT_message("Midi recording buffer full.\nUnless your computer was almost halting because of high CPU usage, "
                         "or your MIDI input and output ports are connected recursively, please report this incident.");
          }
          
        }
      }
      
    } else if (is_note){
      
      midi_event_t midi_event;
      midi_event.timepos = timepos;
      midi_event.msg       = msg;

      // Send event to the pull thread
      if (!g_midi_event_queue.tryPut(midi_event))
        RT_message("Midi recording buffer full.\nUnless your computer was almost halting because of high CPU usage, "
                   "or your MIDI input and output ports are connected recursively, please report this incident.");
    }

  }
}

// A minor hack. The sliders just add pitch wheel midi events to the g_midi_event_queue queue when dragging a recording slider.
void MIDI_add_automation_recording_event(SoundPlugin *plugin, int effect_num, float value){
  midi_event_t midi_event;

  if (is_playing()==false)
    return;

  if (value < 0){
    printf("   Warning: value < 0: %f\n",value);
    value = 0;
  }else if (value > 1.0){
    printf("   Warning: value > 1: %f\n",value);
    value = 1;
  }
  
  if (MIXER_fill_in_time_position(&midi_event.timepos) == true){ // <-- MIXER_fill_in_time_position returns false if the event was recorded after song end, or program is initializing, or there was an error.
    int val;

    if (value < 0.5)
      val = scale(value, 0, 0.5, 0, 0x2000);
    else
      val = scale(value, 0.0, 1.0, 0x2000, 0x3fff);

    // not sure if this is necessary.
    if (val < 0)
      val = 0;
    else if (val > 0x3fff)
      val = 0x3fff;

    midi_event.msg = (0xe0 << 16) | ((val&127) << 8) | (val >> 7);

    midi_event.plugin = plugin;
    midi_event.effect_num = effect_num;
    
    if (!g_midi_event_queue.tryPut(midi_event))
      RT_message("Midi recording buffer full.\nUnless your computer was almost halting because of high CPU usage, "
                 "or your MIDI input and output ports are connected recursively, please report this incident.");
  }
}
                                         
// Runs in its own thread
static void *recording_queue_pull_thread(void*){
  while(true){
    
    midi_event_t event = g_midi_event_queue.get();
    
    {
      radium::ScopedMutex lock(&g_midi_event_mutex);

      // Schedule "Seq" painting
      if (ATOMIC_GET(root->song_state_is_locked) == true){ // not totally tsan proof, but we use the _r0 versions of ListFindElement below, so it's pretty safe.
        struct Blocks *block = (struct Blocks*)ListFindElement1_r0(&root->song->blocks->l, event.timepos.blocknum);
        if (block != NULL){          
          struct Tracks *track = (struct Tracks*)ListFindElement1_r0(&block->tracks->l, event.timepos.tracknum);
          if (track != NULL){
            if (ATOMIC_GET(track->is_recording) == false){
              GFX_ScheduleEditorRedraw();
              ATOMIC_SET(track->is_recording, true);
            }
          }
        }
      }

      // Send event to the main thread
      g_recorded_midi_events.push_back(event);
    }
    
  }

  return NULL;
}


static midi_event_t *find_midievent_end_note(int blocknum, int pos, int notenum_to_find, STime starttime_of_note){
  R_ASSERT(g_midi_event_mutex.is_locked());
  
  for(int i = pos ; i < g_recorded_midi_events.size(); i++) {

    midi_event_t *midi_event = g_recorded_midi_events.ref(i);
    
    if (midi_event->timepos.blocknum != blocknum)
      return NULL;

    uint32_t msg     = midi_event->msg;
    int      notenum = MIDI_msg_byte2(msg);
    
    if (msg_is_note_off(msg)){
      if (notenum==notenum_to_find && midi_event->timepos.blocktime > starttime_of_note)
        return midi_event;
    }
  }

  return NULL;
}


// Happens if note was started in block a, and stopped in block b.
static void add_recorded_stp(struct Blocks *block, struct Tracks *track, const STime time){
        
  Place place = STime2Place(block,time);
        
  struct Stops *stop=(struct Stops*)talloc(sizeof(struct Stops));
  PlaceCopy(&stop->l.p,&place);
          
  ListAddElement3(&track->stops,&stop->l);
}


static void add_recorded_note(struct WBlocks *wblock, struct Blocks *block, struct WTracks *wtrack, const int recorded_midi_events_pos, const STime time, const uint32_t msg){
        
  Place place = STime2Place(block,time);
  int notenum = MIDI_msg_byte2(msg);
  int volume  = MIDI_msg_byte3(msg);
        
  Place endplace;
  Place *endplace_p = NULL; // if NULL, the note doesn't stop in this block.
          
  midi_event_t *midi_event_endnote = find_midievent_end_note(block->l.num, recorded_midi_events_pos+1, notenum, time);
  if (midi_event_endnote != NULL) {
    endplace = STime2Place(block,midi_event_endnote->timepos.blocktime);
    endplace_p = &endplace;
    midi_event_endnote->msg = 0; // don't use this event again later.
  }
  
  InsertNote(wblock,
             wtrack,
             &place,
             endplace_p,
             notenum,
             (float)volume * MAX_VELOCITY / 127.0f,
             true
             );
}


static void add_recorded_fx(struct Tracker_Windows *window, struct WBlocks *wblock, struct Blocks *block, struct WTracks *wtrack, const int recorded_midi_events_pos, const midi_event_t &first_event){

  printf("Add recorded fx %s / %d. %x\n",first_event.plugin->patch->name, first_event.effect_num, first_event.msg);

  int blocknum = wblock->l.num;
  int tracknum = wtrack->l.num;

  struct Tracks *track = wtrack->track;
  const struct Patch *track_patch = track->patch;

  SoundPlugin *plugin = first_event.plugin;
  const int effect_num = first_event.effect_num;
  struct Patch *patch = (struct Patch*)plugin->patch;

  if (patch==NULL)
    return;

  if (track_patch==NULL) {
    setInstrumentForTrack(patch->id, tracknum, blocknum, -1);
    track_patch = patch;
  }
  
  if (track_patch->instrument != get_audio_instrument())
    return;

  Place place = STime2Place(block,first_event.timepos.blocktime);
  float value = get_msg_fx_value(first_event.msg);
  const char *effect_name = PLUGIN_get_effect_name(plugin, effect_num);

  bool next_node_must_be_set = false;
  
  int fxnum = getFx(effect_name, tracknum, patch->id, blocknum, -1);
  if (fxnum==-1)
    return;

  ADD_UNDO(FXs(window, block, track, wblock->curr_realline));

  Undo_start_ignoring_undo_operations();
  
  if (fxnum==-2){
    fxnum = createFx(value, place, effect_name, tracknum, patch->id, blocknum, -1);
    setFxnodeLogtype(LOGTYPE_HOLD, 0, fxnum, tracknum, blocknum, -1);
    next_node_must_be_set = true;
  }else{
    int nodenum = createFxnode(value, place, fxnum, tracknum, blocknum, -1);
    setFxnodeLogtype(LOGTYPE_HOLD, nodenum, fxnum, tracknum, blocknum, -1);
  }
  
  for(int i = recorded_midi_events_pos+1 ; i < g_recorded_midi_events.size(); i++) {
    
    midi_event_t *midi_event = g_recorded_midi_events.ref(i);

    if (midi_event->timepos.blocknum != blocknum)
      break;
    
    if (midi_event->timepos.tracknum != tracknum)
      continue;

    if (!msg_is_fx(midi_event->msg))
      continue;
    
    if (midi_event->plugin != plugin)
      continue;

    if (midi_event->effect_num != effect_num)
      continue;

    Place place = STime2Place(block,midi_event->timepos.blocktime);
    float value = get_msg_fx_value(midi_event->msg);

    int nodenum;

    if (next_node_must_be_set){
      nodenum = 1;
      setFxnode(nodenum, value, place, fxnum, tracknum, blocknum, -1);
      next_node_must_be_set = false;
    } else {
      nodenum = createFxnode(value, place, fxnum, tracknum, blocknum, -1);
    }

    if (nodenum != -1)
      setFxnodeLogtype(LOGTYPE_HOLD, nodenum, fxnum, tracknum, blocknum, -1);
          
    midi_event->msg = 0; // don't use again later.
  }

  Undo_stop_ignoring_undo_operations();
}

// Called from the main thread after the player has stopped
void MIDI_insert_recorded_midi_events(void){
  while(g_midi_event_queue.size() > 0) // Wait til the recording_queue_pull_thread is finished draining the queue.
    usleep(1000*5);

  ATOMIC_SET(root->song_state_is_locked, false);
         
  {
    radium::ScopedMutex lock(&g_midi_event_mutex);
    printf("MIDI_insert_recorded_midi_events called %d\n", g_recorded_midi_events.size());
    if (g_recorded_midi_events.size() == 0)
      goto exit;
  }

  
  usleep(1000*20); // Wait a little bit more for the last event to be transfered into g_recorded_midi_events. (no big deal if we lose it though, CPU is probably so buzy if that happens that the user should expect not everything working as it should. It's also only in theory that we could lose the last event since the transfer only takes some nanoseconds, while here we wait 20 milliseconds.)


  {
    radium::ScopedMutex lock(&g_midi_event_mutex);
    radium::ScopedUndo scoped_undo;

    struct Tracker_Windows *window = root->song->tracker_windows;
    
    hash_t *track_set = HASH_create(8);

    for(int i = 0 ; i < g_recorded_midi_events.size(); i++) { // end note events can be removed from g_recorded_midi_events inside this loop
      
      auto midi_event = g_recorded_midi_events[i];

      //printf("%d / %d: %x\n",midi_event.timepos.blocknum, midi_event.timepos.tracknum, midi_event.msg);
        
      if (midi_event.timepos.tracknum >= 0 && midi_event.msg > 0) {

        // Find block and track
        //
        struct WBlocks *wblock = (struct WBlocks*)ListFindElement1(&window->wblocks->l, midi_event.timepos.blocknum);
        R_ASSERT_RETURN_IF_FALSE(wblock!=NULL);

        struct WTracks *wtrack;

        if (midi_event.timepos.tracknum < 0)
          wtrack = wblock->wtracks;
        if (midi_event.timepos.tracknum >= wblock->block->num_tracks)
          wtrack = (struct WTracks*)ListLast1(&wblock->wtracks->l);
        else {
          wtrack = (struct WTracks*)ListFindElement1(&wblock->wtracks->l, midi_event.timepos.tracknum);
          R_ASSERT_RETURN_IF_FALSE(wtrack!=NULL);
        }
                    
        struct Blocks *block = wblock->block;
        struct Tracks *track = wtrack->track;

        // Update GFX
        //
        ATOMIC_SET(track->is_recording, false);


        // UNDO
        //
        char *key = (char*)talloc_format("%x",track);
        if (HASH_has_key(track_set, key)==false){

          ADD_UNDO(Notes(window,
                         block,
                         track,
                         wblock->curr_realline
                         ));
          
          HASH_put_int(track_set, key, 1);
        }

        // Add Data
        //
        const uint32_t msg = midi_event.msg;
        const int cc0 = MIDI_msg_byte1_remove_channel(msg);
        const int data1 = MIDI_msg_byte2(msg);
        const STime time = midi_event.timepos.blocktime;

        if (cc0==0x80 || (cc0==0x90 && data1==0))
          add_recorded_stp(block, track, time);
        
        else if (cc0==0x90)
          add_recorded_note(wblock, block, wtrack, i, time, msg);

        else if (msg_is_fx(msg))
          add_recorded_fx(window, wblock, block, wtrack, i, midi_event);
      }
      
    }

    g_recorded_midi_events.clear();

  } // end mutex and undo scope

 exit:
  MIXER_set_all_plugins_to_not_recording();
}


/*********************************************************
 *********************************************************
 **                  MIDI Learn                         **
 *********************************************************
 *********************************************************/

void MIDI_add_midi_learn(MidiLearn *midi_learn){
  g_midi_learns.ensure_there_is_room_for_one_more_without_having_to_allocate_memory();

  {
    radium::ScopedMutex lock(&g_midi_learns_mutex); // obtain this lock first to avoid priority inversion
    PLAYER_lock();{
      g_midi_learns.push_back(midi_learn);
    }PLAYER_unlock();
  }
  
  g_midi_learns.post_add();

  MIDILEARN_PREFS_add(midi_learn);
}

void MIDI_remove_midi_learn(MidiLearn *midi_learn, bool show_error_if_not_here){
  R_ASSERT(show_error_if_not_here==false);
  
  MIDILEARN_PREFS_remove(midi_learn);
  
  for(auto midi_learn2 : g_midi_learns)
    if (midi_learn == midi_learn2) {
      radium::ScopedMutex lock(&g_midi_learns_mutex); // obtain this lock first to avoid priority inversion
      PLAYER_lock();{
        g_midi_learns.remove(midi_learn2);
      }PLAYER_unlock();
      return;
    }

  if (show_error_if_not_here==false)
    RError("MIDI_remove_midi_learn: midi_learn not found");
}



/****************************************************************************
*****************************************************************************
 **          Send MIDI input to midi learn patch / current patch           **
 ****************************************************************************
 ****************************************************************************/

hash_t* MidiLearn::create_state(void){
  hash_t *state = HASH_create(3);
  HASH_put_bool(state, "is_enabled", ATOMIC_GET(is_enabled));
  HASH_put_bool(state, "is_learning", ATOMIC_GET(is_learning));
  HASH_put_chars(state, "port_name", ATOMIC_GET(port_name)==NULL ? "" : ATOMIC_GET(port_name)->name);
  HASH_put_int(state, "byte1", ATOMIC_GET(byte1));
  HASH_put_int(state, "byte2", ATOMIC_GET(byte2));
  return state;
}

void MidiLearn::init_from_state(hash_t *state){
  ATOMIC_SET(byte2, HASH_get_int(state, "byte2"));
  ATOMIC_SET(byte1, HASH_get_int(state, "byte1"));
  ATOMIC_SET(port_name, get_symbol(HASH_get_chars(state, "port_name")));
  ATOMIC_SET(is_learning, HASH_get_bool(state, "is_learning"));
  ATOMIC_SET(is_enabled, HASH_get_bool(state, "is_enabled"));
}

bool MidiLearn::RT_matching(const symbol_t *port_name, uint32_t msg){

  if (ATOMIC_GET(is_enabled)==false)
    return false;
  
  int d1 = MIDI_msg_byte1(msg);
  int d2 = MIDI_msg_byte2(msg);

  if (d1 < 0xb0)
    return false;

  if (d1 >= 0xc0 && d1 < 0xe0)
    return false;

  if (d1 >= 0xf0)
    return false;

  if (ATOMIC_GET(is_learning)){
    ATOMIC_SET(byte1, d1);
    ATOMIC_SET(byte2, d2);
    ATOMIC_SET(is_learning, false);
    ATOMIC_SET(this->port_name, port_name);
  }

  //printf("Got msg %x. byte1: %x, byte2: %x\n", msg, byte1, byte2);

  if(ATOMIC_GET(this->port_name) != port_name)
    return false;

  if (d1 < 0xc0) {

    // cc
    
    if(d1==ATOMIC_GET(byte1) && d2==ATOMIC_GET(byte2))
      return true;

  } else {
    
    // pitch bend

    if (d1==ATOMIC_GET(byte1))
      return true;
  }

  return false;
}

bool MidiLearn::RT_maybe_use(const symbol_t *port_name, uint32_t msg){  
  if (RT_matching(port_name, msg)==false)
    return false;

  float value = get_msg_fx_value(msg);

  RT_callback(value);
  
  return true;
}

typedef struct {
  const symbol_t *port_name;
  int32_t deltatime;
  uint32_t msg;
} play_buffer_event_t;

static boost::lockfree::queue<play_buffer_event_t, boost::lockfree::capacity<8000> > g_play_buffer;

// Called from the player thread
void RT_MIDI_handle_play_buffer(void){
  struct Patch *through_patch = ATOMIC_GET(g_through_patch);

  play_buffer_event_t event;

  while(g_play_buffer.pop(event)==true){

    uint32_t msg = event.msg;
    
    for (auto midi_learn : g_midi_learns) {
      midi_learn->RT_maybe_use(event.port_name, msg);
    }
    
    if(through_patch!=NULL){

      uint8_t byte[3] = {(uint8_t)MIDI_msg_byte1(msg), (uint8_t)MIDI_msg_byte2(msg), (uint8_t)MIDI_msg_byte3(msg)};

      RT_MIDI_send_msg_to_patch((struct Patch*)through_patch, byte, 3, -1);
    }
    
  }
}

// Called from a MIDI input thread
static void add_event_to_play_buffer(const symbol_t *port_name, uint32_t msg){
  play_buffer_event_t event;

  event.port_name = port_name;
  event.deltatime = 0;
  event.msg = msg;

  if (!g_play_buffer.bounded_push(event))
    RT_message("Midi play buffer full.\nMost likely the player can not keep up because it uses too much CPU.\nIf that is not the case, please report this incident.");
}


// This is safe. A patch is never deleted.
void MIDI_SetThroughPatch(struct Patch *patch){
  //printf("Sat new patch %p\n",patch);
  if(patch!=NULL)
    ATOMIC_SET(g_through_patch, patch);
}




/*********************************************************
 *********************************************************
 **       Got MIDI from the outside. Entry point.       **
 *********************************************************
 *********************************************************/

// Can be called from any thread
void MIDI_InputMessageHasBeenReceived(const symbol_t *port_name, int cc,int data1,int data2){
  //printf("got new message. on/off:%d. Message: %x,%x,%x\n",(int)root->editonoff,cc,data1,data2);
  //static int num=0;
  //num++;

  if(cc==0xf0 || cc==0xf7) // sysex not supported
    return;
  
  if (MIXER_is_saving())
    return;
  
  bool isplaying = is_playing();

  uint32_t msg = MIDI_msg_pack3(cc, data1, data2);
  int len = MIDI_msg_len(msg);
  if (len<1 || len>3)
    return;
  
  add_event_to_play_buffer(port_name, msg);

  if (g_record_accurately_while_playing && isplaying) {
    
    if (ATOMIC_GET(root->editonoff))
      record_midi_event(port_name, msg);

  } else {

    if(msg_is_note_on(msg))
      if (ATOMIC_COMPARE_AND_SET_UINT32(g_msg, 0, msg)==false) {
        // printf("Playing to fast. Skipping note %u from MIDI input.\n",msg); // don't want to print in realtime thread
      }
  }
}



/*************************************************************
**************************************************************
 **  Insert MIDI received from the outside into the editor  **
 *************************************************************
 *************************************************************/


// called very often
void MIDI_HandleInputMessage(void){
  // should be a memory barrier here somewhere.

  uint32_t msg = ATOMIC_GET(g_msg); // Hmm, should have an ATOMIC_COMPAREFALSE_AND_SET function. (doesn't matter though, it would just look better)
  
  if (msg!=0) {

    ATOMIC_SET(g_msg, 0);

    if(ATOMIC_GET(root->editonoff)){
      float velocity = -1.0f;
      if (g_record_velocity)
        velocity = (float)MIDI_msg_byte3(msg) / 127.0;
      //printf("velocity: %f, byte3: %d\n",velocity,MIDI_msg_byte3(msg));
      InsertNoteCurrPos(root->song->tracker_windows,MIDI_msg_byte2(msg), false, velocity);
      root->song->tracker_windows->must_redraw = true;
    }
  }
}



/*************************************************************
**************************************************************
 **                     Initialization                      **
 *************************************************************
 *************************************************************/

void MIDI_input_init(void){
  radium::ScopedMutex lock(&g_midi_event_mutex);
    
  MIDI_set_record_accurately(SETTINGS_read_bool("record_midi_accurately", true));
  MIDI_set_record_velocity(SETTINGS_read_bool("always_record_midi_velocity", false));
  
  static pthread_t recording_pull_thread;
  pthread_create(&recording_pull_thread, NULL, recording_queue_pull_thread, NULL);
}
