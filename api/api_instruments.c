/* Copyright 2001-2012 Kjetil S. Matheussen

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

#include "Python.h"
#include "radium_proc.h"

#include <string.h>
#include "../common/nsmtracker.h"
#include "../common/list_proc.h"
#include "../common/vector_proc.h"
#include "../common/OS_visual_input.h"
#include "../common/undo.h"
#include "../common/undo_tracks_proc.h"
#include "../common/gfx_wtrackheaders_proc.h"

#include "../embedded_scheme/s7extra_proc.h"

#include "../midi/midi_i_plugin.h"
#include "../midi/midi_i_plugin_proc.h"
#include "../midi/midi_i_input_proc.h"
#include "../midi/midi_menues_proc.h"

#include "../audio/SoundPlugin.h"
#include "../audio/SoundPlugin_proc.h"
#include "../audio/AudioMeterPeaks_proc.h"
#include "../audio/SoundProducer_proc.h"
#include "../audio/SoundPluginRegistry_proc.h"
#include "../audio/Mixer_proc.h"
#include "../audio/Sampler_plugin_proc.h"
#include "../audio/audio_instrument_proc.h"
#include "../audio/Presets_proc.h"
#include "../audio/undo_audio_effect_proc.h"
#include "../audio/undo_audio_connection_volume_proc.h"

#include "../mixergui/QM_MixerWidget.h"
#include "../mixergui/QM_chip.h"
#include "../mixergui/undo_chip_position_proc.h"
//#include "../mixergui/undo_chip_addremove_proc.h"
#include "../mixergui/undo_mixer_connections_proc.h"
#include "../mixergui/undo_mixer_proc.h"

#include "../OpenGL/Render_proc.h"

#include "../Qt/Qt_instruments_proc.h"

#include "../common/patch_proc.h"
#include "../common/instruments_proc.h"
#include "../common/settings_proc.h"

#include "api_common_proc.h"


extern struct Root *root;



DEFINE_ATOMIC(bool, g_enable_autobypass) = false;
DEFINE_ATOMIC(int, g_autobypass_delay) = 500;


bool autobypassEnabled(void){
  static bool has_inited = false;

  if (has_inited==false){
    ATOMIC_SET(g_enable_autobypass, SETTINGS_read_bool("enable_autobypass", false));
    has_inited = true;
  }

  return ATOMIC_GET(g_enable_autobypass);
}

void setAutobypassEnabled(bool doit){
  if (doit != ATOMIC_GET(g_enable_autobypass)) {
    ATOMIC_SET(g_enable_autobypass, doit);
    SETTINGS_write_bool("enable_autobypass", doit);
    PREFERENCES_update();
  }
}

int getAutoBypassDelay(void){
  static bool has_inited = false;

  if (has_inited==false){
    ATOMIC_SET(g_autobypass_delay, SETTINGS_read_int32("autobypass_delay", 500));
    has_inited = true;
  }

  return ATOMIC_GET(g_autobypass_delay);
}

void setAutobypassDelay(int val){
  if (val != ATOMIC_GET(g_autobypass_delay)) {
    ATOMIC_SET(g_autobypass_delay, val);
    SETTINGS_write_int("autobypass_delay", val);
    PREFERENCES_update();
  }
}



// Warning, All these functions (except selectPatchForTrack) must be called via python (does not update graphics, or handle undo/redo))
// (TODO: detect this automatically.)


void selectInstrumentForTrack(int tracknum){
  s7extra_callFunc2_void_int("select-track-instrument", tracknum);
}

void replaceInstrument(int64_t instrument_id, const_char* instrument_description){
  s7extra_callFunc2_void_int_charpointer("replace-instrument", instrument_id, instrument_description);
}

void loadInstrumentPreset(int64_t instrument_id, const_char* instrument_description){
  s7extra_callFunc2_void_int_charpointer("load-instrument-preset", instrument_id, instrument_description);
}

int64_t getInstrumentForTrack(int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	tracknum
	);

  if(wtrack==NULL) return -1;

  struct Patch *patch = wtrack->track->patch;

  if (patch==NULL)
    return -2;

  return patch->id;
}

void setInstrumentForTrack(int64_t instrument_id, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	tracknum
	);

  if(wtrack==NULL) return;

  struct Patch *new_patch = getPatchFromNum(instrument_id);
  if(new_patch==NULL)
    return;

  struct Patch *old_patch = wtrack->track->patch;

  if (new_patch==old_patch)
    return;

  ADD_UNDO(Track(window,wblock,wtrack,wblock->curr_realline));

  PLAYER_lock();{
    
    if (old_patch != NULL)
      handle_fx_when_theres_a_new_patch_for_track(wtrack->track, old_patch, new_patch);
    
    wtrack->track->patch = new_patch;
    
  }PLAYER_unlock();
  
  wblock->block->is_dirty = true;

  (*new_patch->instrument->PP_Update)(new_patch->instrument,new_patch);
}


DEFINE_ATOMIC(bool, g_use_track_channel_for_midi_input) = true;

bool doUseTrackChannelForMidiInput(void){
  static DEFINE_ATOMIC(bool, has_inited) = false;

  if (ATOMIC_GET(has_inited)==false){
    ATOMIC_SET(g_use_track_channel_for_midi_input, SETTINGS_read_bool("use_track_channel_for_midi_input", true));
    ATOMIC_SET(has_inited, true);
  }

  return ATOMIC_GET(g_use_track_channel_for_midi_input);
}

void setUseTrackChannelForMidiInput(bool doit){
  ATOMIC_SET(g_use_track_channel_for_midi_input, doit);
  SETTINGS_write_bool("use_track_channel_for_midi_input", doit);
}


int64_t createMIDIInstrument(char *name) {
  struct Patch *patch = PATCH_create_midi(name);
  GFX_PP_Update(patch);
  return patch->id;
}

// There was a good reason for the 'name' parameter. Think it had something to do with replace instrument, and whether to use old name or autogenerate new one.
int64_t createAudioInstrument(char *type_name, char *plugin_name, char *name, float x, float y) {
  printf("createAudioInstrument called\n");
  
  if (name!=NULL && strlen(name)==0)
    name = NULL;

  struct Patch *patch = PATCH_create_audio(type_name, plugin_name, name, NULL, x, y);
  if (patch==NULL)
    return -1;

  //MW_move_chip_to_slot(patch, x, y); // Ensure it is placed in a slot. (x and y comes from mouse positions, which are not necessarily slotted). <--- Changed. x and y must be slotted before calling this function.
  
  {
    struct SoundPlugin *plugin = patch->patchdata;
    inc_plugin_usage_number(plugin->type);
  }

  return patch->id;
}

int64_t createAudioInstrumentFromPreset(const char *filename, char *name, float x, float y) {
  return PRESET_load(STRING_create(filename), name, false, x, y);
}

int64_t createAudioInstrumentFromDescription(const char *instrument_description, char *name, float x, float y){
  if (strlen(instrument_description)==0)
    return -1;

  if (name!=NULL && strlen(name)==0)
    name = NULL;

  if (instrument_description[0]=='1'){
    
    char *descr = talloc_strdup(instrument_description);
    int sep_pos = 1;
    while(descr[sep_pos]!=':'){
      if(descr[sep_pos]==0){
        handleError("Illegal instrument_description: %s (missing colon separator)",instrument_description);
        return -1;
      }
      sep_pos++;
    }
    descr[sep_pos] = 0;
    char *type_name = STRING_get_chars(STRING_fromBase64(STRING_create(&descr[1])));
    char *plugin_name = STRING_get_chars(STRING_fromBase64(STRING_create(&descr[sep_pos+1])));
    return createAudioInstrument(type_name, plugin_name, name, x, y);
    
  } else if (instrument_description[0]=='2'){
    
    wchar_t *filename = STRING_fromBase64(STRING_create(&instrument_description[1]));
    //printf("filename: %s\n",filename);

    return PRESET_load(filename, name, true, x, y);
    
  } else if (instrument_description[0]=='3'){

    return MW_paste(x, y);
        
  } else {

    handleError("Illegal instrument_description: %s (string doesn't start with '1', '2' or '3')",instrument_description);
    return -1;

  }
}

int64_t cloneAudioInstrument(int64_t instrument_id, float x, float y){
  struct Patch *old_patch = getAudioPatchFromNum(instrument_id);
  if(old_patch==NULL)
    return -1;
  
  hash_t *state = PATCH_get_state(old_patch);

  struct Patch *new_patch = PATCH_create_audio(NULL, NULL, talloc_format("Clone of %s",old_patch->name), state, x, y);
  if (new_patch==NULL)
    return -1;

  return new_patch->id;
}

void connectAudioInstrumentToMainPipe(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return;
  }

  ADD_UNDO(MixerConnections_CurrPos());
  MW_autoconnect_plugin(plugin);
}

const_char* instrumentDescriptionPopupMenu(void){
  return MW_popup_plugin_selector2();
}

const_char* requestLoadPresetInstrumentDescription(void){
  return PRESET_request_load_instrument_description();
}

int getNumInstrumentEffects(int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  return patch->instrument->getFxNames(patch)->num_elements;
}

const_char* getInstrumentEffectName(int effect_num, int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return "";

  vector_t *elements = patch->instrument->getFxNames(patch);
  
  if (effect_num >= elements->num_elements){
    handleError("effect_num >= num_effects: %d >= %d",effect_num, elements->num_elements);
    return "";
  }
    
  return talloc_strdup(elements->elements[effect_num]);
}

bool hasPureData(void){
#if WITH_PD
  return true;
#else
  return false;
#endif
}

void setInstrumentSample(int64_t instrument_id, char *filename){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return;
  }

  if (strcmp(plugin->type->name, "Sample Player")) {
    handleError("instrument %d is not a Sample Player", instrument_id);
    return;
  }


  SAMPLER_set_new_sample(plugin, STRING_create(filename), -1);
}

void setInstrumentLoopData(int64_t instrument_id, int start, int length){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return;
  }

  if (strcmp(plugin->type->name, "Sample Player")) {
    handleError("instrument %d is not a Sample Player", instrument_id);
    return;
  }


  SAMPLER_set_loop_data(plugin, start, length);

  GFX_update_instrument_widget(patch);

}

char *getInstrumentName(int64_t instrument_id) {
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return "";

  return (char*)patch->name;
}

void setInstrumentName(char *name, int64_t instrument_id) {
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  PATCH_set_name(patch, name);
  patch->name_is_edited = true;
  
  (*patch->instrument->PP_Update)(patch->instrument,patch);
}

const_char* getInstrumentComment(int64_t instrument_id) {
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return "";

  return patch->comment;
}

void setInstrumentComment(const_char* comment, int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  patch->comment = talloc_strdup(comment);
}

bool instrumentNameWasAutogenerated(int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return false;

  return !patch->name_is_edited;
}

void setInstrumentColor(const_char *colorname, int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  unsigned int color = GFX_get_color_from_colorname(colorname);
  patch->color = color;

  if (patch->instrument==get_audio_instrument()){
    struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
    if (plugin==NULL){
      handleError("Instrument #d has been closed", (int)instrument_id);
      return;
    }
    CHIP_update(plugin);
  }

  GFX_ScheduleRedraw();
}

const char *getInstrumentColor(int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return "";

  return GFX_get_colorname_from_color(patch->color);
}

float getInstrumentEffect(int64_t instrument_id, const_char* effect_name){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return 0.0;
  }

  return PLUGIN_get_effect_from_name(plugin, effect_name);
}


void setInstrumentEffect(int64_t instrument_id, const char *effect_name, float value){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return;
  }

  /*
  if (strcmp(plugin->type->name, "Sample Player")) {
    handleError("instrument %d is not a Sample Player plugin", instrument_id);
    return;
  }
  */

  PLUGIN_set_effect_from_name(plugin, effect_name, value);

  if (!strcmp(effect_name, "System Volume") ||
      !strcmp(effect_name, "System In") ||
      !strcmp(effect_name, "System Solo On/Off") ||
      !strcmp(effect_name, "System Volume On/Off") ||
      !strcmp(effect_name, "System Effects On/Off"))
    CHIP_update(plugin);
      
  GFX_update_instrument_widget(patch);
}

void undoInstrumentEffect(int64_t instrument_id, const char *effect_name){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return;
  }

  int effect_num = PLUGIN_get_effect_num(plugin, effect_name);

  if (effect_num==-1){
    handleError("");
    return;
  }
  
  ADD_UNDO(AudioEffect_CurrPos(patch, effect_num));
}

#if 0
void setInstrumentVolume(int64_t instrument_id, float volume) {
  struct Instruments *instrument = getInstrumentFromNum(instrument_id);
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL) return NULL;

  (*patch->instrument->PP_Update)(instrument,patch);
}

float getInstrumentVolume(int64_t instrument_id) {
  return 0.0f;
}
#endif

float getMinDb(void){
  return MIN_DB;
}

float getMaxDb(void){
  return MAX_DB;
}


void setInstrumentData(int64_t instrument_id, char *key, char *value) {
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  patch->instrument->setPatchData(patch, key, value);

  (*patch->instrument->PP_Update)(patch->instrument,patch);
}

char *getInstrumentData(int64_t instrument_id, char *key) {
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return "";

  return patch->instrument->getPatchData(patch, key);
}


int getNumMIDIInstruments(void){
  return get_MIDI_instrument()->patches.num_elements;
}

int64_t getMIDIInstrumentId(int instrument_num){
  if (instrument_num>=getNumMIDIInstruments()){
    handleError("No instrument #%d",instrument_num);
    return -1;
  }
  struct Patch *patch = get_MIDI_instrument()->patches.elements[instrument_num];
  return patch->id;
}

int getNumAudioInstruments(void){
  return get_audio_instrument()->patches.num_elements;
}

int64_t getAudioInstrumentId(int instrument_num){
  if (instrument_num>=getNumAudioInstruments()){
    handleError("No instrument #%d",instrument_num);
    return -1;
  }
  struct Patch *patch = get_audio_instrument()->patches.elements[instrument_num];
  return patch->id;
}

int64_t getAudioBusId(int bus_num){
  if (bus_num < 0 || bus_num>=5){
    handleError("There is no bus %d", bus_num);
    return -1;
  }

  struct Patch *patch = MIXER_get_bus(bus_num);
  if (patch==NULL)
    return -1; // never happens
  
  return patch->id;
}

bool instrumentIsBusDescendant(int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return false;

  if (patch->instrument == get_audio_instrument()){
    struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
    if (plugin==NULL){
      handleError("Instrument #d has been closed", (int)instrument_id);
      return true;
    }

    struct SoundProducer *sp = SP_get_sound_producer(plugin);
    R_ASSERT_RETURN_IF_FALSE2(sp!=NULL, false);
    return SP_get_bus_descendant_type(sp)==IS_BUS_DESCENDANT;
  }else
    return false; // Can not delete midi instruments.
}

bool instrumentIsPermanent(int64_t instrument_id){  
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return false;

  if (patch->instrument == get_audio_instrument())
    return AUDIO_is_permanent_patch(patch);
  else
    return true; // Can not delete midi instruments.
}

bool instrumentIsAudio(int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return false;

  return patch->instrument == get_audio_instrument();
}

// Mixer GUI
float getInstrumentX(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0.0;

  return CHIP_get_pos_x(patch);
}

float getInstrumentY(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0.0;

  return CHIP_get_pos_y(patch);
}

void setInstrumentPosition(float x, float y, int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  ADD_UNDO(ChipPos_CurrPos(patch));
  
  CHIP_set_pos(patch,x,y);
}

void autopositionInstrument(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return;
  
  CHIP_autopos(patch);
}

int getNumInAudioConnections(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  return CHIP_get_num_in_connections(patch);
}

int getNumInEventConnections(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  return CHIP_get_num_in_econnections(patch);
}

int getNumOutAudioConnections(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  return CHIP_get_num_out_connections(patch);
}

int getNumOutEventConnections(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  return CHIP_get_num_out_econnections(patch);
}

int64_t getAudioConnectionSourceInstrument(int connectionnum, int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  struct Patch *source = CHIP_get_source(patch, connectionnum);
  if (source == NULL)
    return 0;
    
  return source->id;
}

int64_t getEventConnectionSourceInstrument(int connectionnum, int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  struct Patch *source = CHIP_get_esource(patch, connectionnum);
  if (source == NULL)
    return 0;
    
  return source->id;
}

int64_t getAudioConnectionDestInstrument(int connectionnum, int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  struct Patch *dest = CHIP_get_dest(patch, connectionnum);
  if (dest == NULL)
    return 0;
  
  return dest->id;
}

int64_t getEventConnectionDestInstrument(int connectionnum, int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  struct Patch *dest = CHIP_get_edest(patch, connectionnum);
  if (dest == NULL)
    return 0;
  
  return dest->id;
}

void createAudioConnection(int64_t source_id, int64_t dest_id){
  struct Patch *source = getAudioPatchFromNum(source_id);
  if(source==NULL)
    return;

  struct Patch *dest = getAudioPatchFromNum(dest_id);
  if(dest==NULL)
    return;

  MW_connect(source, dest); 
}

float getAudioConnectionVolume(int64_t source_id, int64_t dest_id){
  struct Patch *source = getAudioPatchFromNum(source_id);
  if(source==NULL)
    return 0.0;

  struct Patch *dest = getAudioPatchFromNum(dest_id);
  if(dest==NULL)
    return 0.0;

  struct SoundPlugin *source_plugin = (struct SoundPlugin*)source->patchdata;
  if (source_plugin==NULL){
    handleError("Instrument #d has been closed", (int)source_id);
    return 0.0;
  }

  struct SoundPlugin *dest_plugin = (struct SoundPlugin*)dest->patchdata;
  if (dest_plugin==NULL){
    handleError("Instrument #d has been closed", (int)dest_id);
    return 0.0;
  }

  struct SoundProducer *source_sp = SP_get_sound_producer(source_plugin);
  struct SoundProducer *dest_sp = SP_get_sound_producer(dest_plugin);

  char *error = NULL;

  float ret = SP_get_link_volume(dest_sp, source_sp, &error);

  if (error!=NULL)
    handleError("Could not find audio connection between instrument %d and instrument %d", source_id, dest_id);
  
  return gain2db(ret);
}

void setAudioConnectionVolume(int64_t source_id, int64_t dest_id, float db){
  struct Patch *source = getAudioPatchFromNum(source_id);
  if(source==NULL)
    return;

  struct Patch *dest = getAudioPatchFromNum(dest_id);
  if(dest==NULL)
    return;

  struct SoundPlugin *source_plugin = (struct SoundPlugin*)source->patchdata;
  if (source_plugin==NULL){
    handleError("Instrument #d has been closed", (int)source_id);
    return;
  }

  struct SoundPlugin *dest_plugin = (struct SoundPlugin*)dest->patchdata;
  if (dest_plugin==NULL){
    handleError("Instrument #d has been closed", (int)dest_id);
    return;
  }

  struct SoundProducer *source_sp = SP_get_sound_producer(source_plugin);
  struct SoundProducer *dest_sp = SP_get_sound_producer(dest_plugin);

  char *error = NULL;

  float gain = db2gain(db);
  SP_set_link_volume(dest_sp, source_sp, gain, &error);

  if (error!=NULL)
    handleError("Could not find audio connection between instrument %d and instrument %d", source_id, dest_id);
}


void undoAudioConnectionVolume(int64_t source_id, int64_t dest_id){
  struct Patch *source = getAudioPatchFromNum(source_id);
  if(source==NULL)
    return;

  struct Patch *dest = getAudioPatchFromNum(dest_id);
  if(dest==NULL)
    return;

  ADD_UNDO(AudioConnectionVolume_CurrPos(source, dest));
}


void createEventConnection(int64_t source_id, int64_t dest_id){
  struct Patch *source = getAudioPatchFromNum(source_id);
  if(source==NULL)
    return;

  struct Patch *dest = getAudioPatchFromNum(dest_id);
  if(dest==NULL)
    return;

  MW_econnect(source, dest); 
}
                           
int getNumInputChannels(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return 0;
  }

  return plugin->type->num_inputs;
}

int getNumOutputChannels(int64_t instrument_id){
  struct Patch *patch = getAudioPatchFromNum(instrument_id);
  if(patch==NULL)
    return 0;

  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return 0;
  }

  return plugin->type->num_outputs;
}

void deleteInstrument(int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  Undo_Open_rec();{

    PATCH_make_inactive(patch);

  }Undo_Close();

  root->song->tracker_windows->must_redraw=true;
}

const_char* getSampleBookmarks(int num){
  return SETTINGS_read_string(talloc_format("sample_bookmarks%d",num), "/");
}

void setSampleBookmarks(int num, char* path){
  SETTINGS_write_string(talloc_format("sample_bookmarks%d",num), path);
}

void midi_resetAllControllers(void){
  printf("midi_resetAllControllers called\n");
  MIDIResetAllControllers();
}

void midi_localKeyboardOn(void){
  MIDILocalKeyboardOn();
}

void midi_localKeyboardOff(void){
  MIDILocalKeyboardOff();
}

void midi_allNotesOff(void){
  MIDIAllNotesOff();
}

void midi_allSoundsOff(void){
  MIDIAllSoundsOff();
}

void midi_recordAccurately(bool accurately){
  MIDI_set_record_accurately(accurately);
}

void midi_alwaysRecordVelocity(bool doit){
  MIDI_set_record_velocity(doit);
}

void midi_setInputPort(void){
  MIDISetInputPort();
}

#define NUM_IDS 2048
static int note_ids_pos = 0;
static int64_t note_ids[NUM_IDS] = {0}; // TODO: Change int to int64_t everywhere in the api.
static float initial_pitches[NUM_IDS] = {0}; // TODO: Change int to int64_t everywhere in the api.

int playNote(float pitch, float velocity, float pan, int midi_channel, int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return -1;

  int ret = note_ids_pos;

  note_ids[ret] = PATCH_play_note(patch, create_note_t(NULL, -1, pitch, velocity, pan, midi_channel, 0));
  initial_pitches[ret] = pitch;
    
  note_ids_pos++;
  if (note_ids_pos==NUM_IDS)
    note_ids_pos = 0;
  
  return ret;
}

void changeNotePitch(float pitch, int note_id, int midi_channel, int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  if (note_id < 0 || note_id >= NUM_IDS) {
    handleError("note_id %d not found", note_id);
    return;
  }

  //printf("change pitch %f %d %d\n",pitch,note_id,instrument_id);
  PATCH_change_pitch(patch, create_note_t(NULL,
                                          note_ids[note_id],
                                          pitch,
                                          0,
                                          pitch,
                                          midi_channel,
                                          0
                                          ));
}

void stopNote(int note_id, int midi_channel, int64_t instrument_id){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL)
    return;

  if (note_id < 0 || note_id >= NUM_IDS) {
    handleError("note_id %d not found", note_id);
    return;
  }

  //printf("stop note %d %d\n",note_id,instrument_id);
  PATCH_stop_note(patch, create_note_t(NULL,
                                       note_ids[note_id],
                                       initial_pitches[note_id],
                                       0,
                                       0,
                                       midi_channel,
                                       0
                                       ));
}


/******** Effect monitors ************/

struct EffectMonitor{
  int64_t id;

  struct Patch *patch;
  
  int64_t instrument_id;
  int effect_num;
  func_t *func;

  float last_value;
};

static int64_t g_effect_monitor_id = 0;
static vector_t g_effect_monitors = {0};

static struct EffectMonitor *find_effect_monitor_from_id(int64_t id){
  VECTOR_FOR_EACH(struct EffectMonitor *effect_monitor, &g_effect_monitors){
    if (effect_monitor->id==id)
      return effect_monitor;
  }END_VECTOR_FOR_EACH;

  return NULL;
}

/*
static struct EffectMonitor *find_effect_monitor(int effect_num, int64_t instrument_id){
  VECTOR_FOR_EACH(struct EffectMonitor *effect_monitor, &g_effect_monitors){
    if (effect_monitor->effect_num==effect_num && effect_monitor->instrument_id==instrument_id)
      return effect_monitor;
  }END_VECTOR_FOR_EACH;

  return NULL;
}
*/

int64_t addEffectMonitor(const char *effect_name, int64_t instrument_id, func_t *func){
  struct Patch *patch = getPatchFromNum(instrument_id);
  if(patch==NULL){
    handleError("There is no instrument #%d", instrument_id);
    return -1;
  }

  if (patch->instrument!=get_audio_instrument()){
    handleError("Can only call addEffectMonitor for audio instruments");
    return -1;
  }
      
  struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
  if (plugin==NULL){
    handleError("Instrument #d has been closed", (int)instrument_id);
    return -1;
  }

  int effect_num = PLUGIN_get_effect_num(plugin, effect_name);

  if (effect_num==-1){
    handleError("");
    return -1;
  }
  
    /*
  if (find_effect_monitor(effect_num, instrument_id) != NULL){
    handleError("There is already an effect monitor for %s / %d", getInstrumentName(instrument_id), effect_num);
    return -1;
  }
    */
    
  struct EffectMonitor *effect_monitor = talloc(sizeof(struct EffectMonitor));

  effect_monitor->id = g_effect_monitor_id++;
  effect_monitor->patch = patch;
  
  effect_monitor->effect_num = effect_num;
  effect_monitor->instrument_id = instrument_id;
  effect_monitor->func = func;

  effect_monitor->last_value = 0;

  VECTOR_push_back(&g_effect_monitors, effect_monitor);

  s7extra_protect(effect_monitor->func);  

  return effect_monitor->id;
}

void removeEffectMonitor(int64_t effect_monitor_id){
  struct EffectMonitor *effect_monitor = find_effect_monitor_from_id(effect_monitor_id);
  if (effect_monitor==NULL){
    handleError("No effect monitor #%d", (int)effect_monitor_id);
    return;
  }

  s7extra_unprotect(effect_monitor->func);
    
  VECTOR_remove(&g_effect_monitors, effect_monitor);
}


void API_instruments_call_regularly(void){
  VECTOR_FOR_EACH(struct EffectMonitor *effect_monitor, &g_effect_monitors){
    struct Patch *patch = effect_monitor->patch;
    struct SoundPlugin *plugin = (struct SoundPlugin*)patch->patchdata;
    if(plugin!=NULL){
      float now = plugin->savable_effect_values[effect_monitor->effect_num];
      if (now != effect_monitor->last_value){
        effect_monitor->last_value = now;
        s7extra_callFunc_void_void(effect_monitor->func);
      }
    }
  }END_VECTOR_FOR_EACH;
}
