/* Copyright 2001 Kjetil S. Matheussen

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

#include "../common/includepython.h"

#include <unistd.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <QVector> // Shortening warning in the QVector header. Temporarily turned off by the surrounding pragmas.
#pragma clang diagnostic pop
#include <QLinkedList>

#include "../bin/packages/s7/s7.h"

#include "../common/nsmtracker.h"
#include "../common/list_proc.h"
#include "../common/placement_proc.h"
#include "../common/velocities_proc.h"
#include "../common/tempos_proc.h"
#include "../common/Signature_proc.h"
#include "../common/LPB_proc.h"
#include "../common/temponodes_proc.h"
#include "../common/fxlines_proc.h"
#include "../common/notes_proc.h"
#include "../common/pitches_proc.h"
#include "../common/wblocks_proc.h"
#include "../common/disk_save_proc.h"
#include "../common/disk_load_proc.h"
#include "../common/lines_proc.h"
#include "../common/reallines_insert_proc.h"
#include "../common/block_properties_proc.h"
#include "../common/track_insert_proc.h"
#include "../common/tracks_proc.h"
#include "../common/wtracks_proc.h"
#include "../common/undo_trackheader_proc.h"
#include "../common/block_insert_proc.h"
#include "../common/block_delete_proc.h"
#include "../common/block_split_proc.h"
#include "../common/eventreciever_proc.h"
#include "../common/visual_proc.h"
#include "../common/OS_settings_proc.h"
#include "../common/OS_visual_input.h"
#include "../common/settings_proc.h"
#include "../common/player_proc.h"
#include "../common/player_pause_proc.h"
#include "../common/undo_blocks_proc.h"
#include "../common/time_proc.h"
#include "../common/seqtrack_proc.h"
#include "../embedded_scheme/scheme_proc.h"
#include "../OpenGL/Widget_proc.h"
#include "../OpenGL/Render_proc.h"
#include "../common/OS_string_proc.h"
#include "../audio/SoundProducer_proc.h"
#include "../audio/Mixer_proc.h"
#include "../audio/Faust_plugins_proc.h"
#include "../mixergui/QM_MixerWidget.h"
#include "../embedded_scheme/s7extra_proc.h"

#include "../common/PriorityQueue.hpp"

#ifdef _AMIGA
#include "Amiga_colors_proc.h"
#endif

#include "api_common_proc.h"

#include "api_various_proc.h"
#include "radium_proc.h"



void editorWindowToFront(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
	GFX_EditorWindowToFront(window);
}
void playListWindowToFront(void){
	GFX_PlayListWindowToFront();
}
void instrumentWindowToFront(void){
	GFX_InstrumentWindowToFront();
}
void helpWindowToFront(void){
	GFX_HelpWindowToFront();
}

void setMixerRotate(float rotate){
  MW_set_rotate(rotate);
}

static bool g_showInstrumentWidgetWhenDoubleClickingSoundObject = true;

bool showInstrumentWidgetWhenDoubleClickingSoundObject(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_showInstrumentWidgetWhenDoubleClickingSoundObject = SETTINGS_read_bool("show_instrument_widget_when_double_clicking_sound_object", true);
    has_inited = true;
  }

  return g_showInstrumentWidgetWhenDoubleClickingSoundObject;
}

void setShowInstrumentWidgetWhenDoubleClickingSoundObject(bool val){
  g_showInstrumentWidgetWhenDoubleClickingSoundObject = val;
  SETTINGS_write_bool("show_instrument_widget_when_double_clicking_sound_object", val);
}


static bool g_showPlaylistDuringStartup = true;

bool showPlaylistDuringStartup(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_showPlaylistDuringStartup = SETTINGS_read_bool("show_playlist_during_startup", true);
    has_inited = true;
  }

  return g_showPlaylistDuringStartup;
}

void setShowPlaylistDuringStartup(bool val){
  g_showPlaylistDuringStartup = val;
  SETTINGS_write_bool("show_playlist_during_startup", val);
}


static bool g_showMixerStripDuringStartup = true;

bool showMixerStripDuringStartup(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_showMixerStripDuringStartup = SETTINGS_read_bool("show_mixer_strip_during_startup", true);
    has_inited = true;
  }

  return g_showMixerStripDuringStartup;
}

void setShowMixerStripDuringStartup(bool val){
  g_showMixerStripDuringStartup = val;
  SETTINGS_write_bool("show_mixer_strip_during_startup", val);
}


static bool g_showMixerStripOnLeftSide = true;

bool showMixerStripOnLeftSide(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_showMixerStripOnLeftSide = SETTINGS_read_bool("show_mixer_strip_on_left_side", true);
    has_inited = true;
  }

  return g_showMixerStripOnLeftSide;
}

void setShowMixerStripOnLeftSide(bool val){
  g_showMixerStripOnLeftSide = val;
  SETTINGS_write_bool("show_mixer_strip_on_left_side", val);
}


void maximizeEditorWindow(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  GFX_MaximizeEditorWindow(window);
}
void minimizeEditorWindow(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  GFX_MinimizeEditorWindow(window);
}

void toggleFullScreen(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;

#if 0
  if(GFX_InstrumentWindowIsVisible()==true)
    GFX_SetMinimalInstrumentWindow();
#endif

  GFX_toggleFullScreen(window);

#if 0  
  if(GFX_InstrumentWindowIsVisible()==true)
    GFX_SetMinimalInstrumentWindow();
#endif
}

void showHideEditor(int windownum){
  //struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  GFX_showHideEditor();
}

void showHideSequencer(void){
  if(GFX_SequencerIsVisible())
    GFX_HideSequencer();
  else
    GFX_ShowSequencer();
}

void switchSequencerPlaylistConfiguration(void){
  bool seq  = GFX_SequencerIsVisible();
  bool play = GFX_PlaylistWindowIsVisible();

  int state = (seq ? 2 : 0) + (play ? 1 : 0);

  switch(state){
    case 0: // 00 -> 01
      GFX_PlayListWindowToFront();
      break;
    case 1: // 01 -> 10
      GFX_ShowSequencer();
      GFX_PlayListWindowToBack();
      break;
    case 2: // 10 -> 11
      GFX_PlayListWindowToFront();
      break;
    case 3: // 11 -> 00
      GFX_HideSequencer();
      GFX_PlayListWindowToBack();
      break;
  }
}

void showHideMixerWidget(void){
  GFX_showHideMixerWidget();
}

void showHideInstrumentWidget(int windownum){
  //struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  //GFX_showHideInstrumentWidget(window);
  if(GFX_InstrumentWindowIsVisible())
    GFX_InstrumentWindowToBack();
  else
    GFX_InstrumentWindowToFront();
}


#if 0
void toggleInstrumentWidgetOnly(void){
  //if(GFX_
}
#endif

void showHidePlaylist(int windownum){  
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  GFX_showHidePlaylist(window);
}

void showHideMixerStrip(int windownum){  
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  GFX_showHideMixerStrip(window);
}

void showHideMenuBar(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  if (GFX_MenuVisible(window))
    GFX_HideMenu(window);
  else
    GFX_ShowMenu(window);
}

void hideMenuBar(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  GFX_HideMenu(window);
}

void showMenuBar(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  GFX_ShowMenu(window);
}


// Switch between:
// 1. Editor alone
// 2. Editor + instrument
// 3. Mixer + instruments
void switchWindowConfiguration(void){
  printf("editor: %d\ninstrument: %d\nmixer: %d\n",GFX_EditorIsVisible(), GFX_InstrumentWindowIsVisible(), GFX_MixerIsVisible());

  // From 1 to 2
  if(GFX_EditorIsVisible()==true && GFX_InstrumentWindowIsVisible()==false && GFX_MixerIsVisible()==false){
    GFX_InstrumentWindowToFront();
    //GFX_SetMinimalInstrumentWindow();
    return;
  }

  // From 2 to 3
  if(GFX_EditorIsVisible()==true && GFX_InstrumentWindowIsVisible()==true && GFX_MixerIsVisible()==false){
    GFX_HideEditor();
    GFX_ShowMixer();
    //GFX_SetMinimalInstrumentWindow();
    return;
  }

  // To 1.
  GFX_ShowEditor();
  GFX_InstrumentWindowToBack();
  GFX_HideMixer();
}

void enableMetronome(bool onoff){
  ATOMIC_SET(root->clickonoff, onoff);
}

void enablePlayCursor(bool onoff){
  ATOMIC_SET(root->play_cursor_onoff, onoff);
  
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  window->must_redraw = true;
}

bool playCursorEnabled(void){
  return ATOMIC_GET(root->play_cursor_onoff);
}

void switchPlayCursorOnOff(void){
  return enablePlayCursor(!playCursorEnabled());
}

void enableEditorFollowsPlayCursor(bool onoff){
  ATOMIC_SET(root->editor_follows_play_cursor_onoff, onoff);
  
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  window->must_redraw = true;
}

void insertReallines(int toinsert,int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  InsertRealLines_CurrPos(window,toinsert);
}

void generalDelete(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  switch(window->curr_track){
  case SIGNATURETRACK:
    RemoveSignaturesCurrPos(window);
    break;
  case LPBTRACK:
    RemoveLPBsCurrPos(window);
    break;
  case TEMPOTRACK:
    RemoveTemposCurrPos(window);
    break;
  case TEMPONODETRACK:
    RemoveAllTempoNodesOnReallineCurrPos(window);
    break;
  default:
    if(window->curr_track_sub>=0) StopVelocityCurrPos(window,0);
    else RemoveNoteCurrPos(window);
  }
}

void insertLines(int toinsert,int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  InsertLines_CurrPos(window,toinsert);
}

void generalReturn(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;

  switch(window->curr_track){
  case SIGNATURETRACK:
    SetSignatureCurrPos(window);
    break;
  case LPBTRACK:
    SetLPBCurrPos(window);
    break;
  case TEMPOTRACK:
    SetTempoCurrPos(window);
    break;
  case TEMPONODETRACK:
    AddTempoNodeCurrPos(window,(float) -0.0f);
    break;
  default:
    printf("curr_track: %d, sub_track: %d\n",window->curr_track,window->curr_track_sub);
    if(window->curr_track>=0){
      if (window->curr_track_sub>=0)
        AddVelocityCurrPos(window);
      else if (window->curr_track_sub==-1)
        EditNoteCurrPos(window);
    }
    break;
  }  
}

int appendBlock(void){
  struct Tracker_Windows *window=getWindowFromNum(-1);
  return AppendWBlock(window)->l.num;
}

void appendTrack(int blocknum){
  struct Tracker_Windows *window=getWindowFromNum(-1);
  struct WBlocks *wblock = getWBlockFromNum(-1, blocknum);if(wblock==NULL) return;
  AppendWTrack_CurrPos(window,wblock);
}

void swapTracks(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  SwapTrack_CurrPos(window);
}

void makeTrackMonophonic(int tracknum, int blocknum, int windownum){
  struct WTracks *wtrack = getWTrackFromNum(windownum, blocknum,tracknum);
  if(wtrack==NULL)
    return;
  
  TRACK_make_monophonic_destructively(wtrack->track);
}

void splitTrackIntoMonophonicTracks(int tracknum, int blocknum, int windownum){
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

  if (TRACK_split_into_monophonic_tracks(window, wblock, wtrack)==false)
    GFX_Message(NULL, "Track #%d is already monophonic", tracknum);
}
  
void splitBlock(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  BLOCK_Split_CurrPos(window);
}


// Warning, must be called via python (does not update graphics or handle undo/redo)
void setNumTracks(int numtracks, int blocknum, int windownum){
  struct Tracker_Windows *window=NULL;
  struct WBlocks *wblock = getWBlockFromNumA(
                                             windownum,
                                             &window,
                                             blocknum
                                             );
  if(wblock==NULL) return;

  Block_Set_num_tracks(wblock->block, numtracks);
  //MinimizeBlock_CurrPos(window);
  wblock->block->is_dirty = true;
}


// Warning, must be called via python (does not update graphics or handle undo/redo)
void setNumLines(int numlines, int blocknum, int windownum){
  struct Tracker_Windows *window=NULL;
  struct WBlocks *wblock = getWBlockFromNumA(
                                             windownum,
                                             &window,
                                             blocknum
                                             );
  if(wblock==NULL) return;

  Block_Set_num_lines(wblock->block, numlines);
  wblock->block->is_dirty = true;
}

void changeTrackNoteLength(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  ChangeNoteLength_CurrPos(window);
}

void changeBlockNoteLength(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  ChangeNoteLength_Block_CurrPos(window);
}

void changeTrackNoteAreaWidth(int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack=getWTrackFromNumA(
                                           windownum,
                                           &window,
                                           blocknum,
                                           &wblock,
                                           tracknum
                                           );
  if(wtrack==NULL) return;

  if (wtrack->is_wide){
    wtrack->fxwidth = wtrack->non_wide_fx_width;
  } else {
    wtrack->non_wide_fx_width = wtrack->fxwidth;
    wtrack->fxwidth *= 2;
    if (wtrack->fxwidth < 100)
      wtrack->fxwidth = 100;
  }
  
  wtrack->is_wide = !wtrack->is_wide;
  
    
  window->must_redraw = true;
}

void changeBlockNoteAreaWidth(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  ChangeNoteAreaWidth_Block_CurrPos(window);
}

void minimizeTrack(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  MinimizeTrack_CurrPos(window);
}

void minimizeBlockTracks(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  MinimizeBlock_CurrPos(window);
}

extern bool doquit;

void quit(void){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  doquit=Quit(window);
  if(doquit==true) printf("doquit is really true.\n");
}

extern LANGSPEC void SOUNDFILESAVERGUI_open(void);
void saveSoundfile(void){
  SOUNDFILESAVERGUI_open();
}

extern LANGSPEC void COMMENTDIALOG_open(void);
void openCommentDialog(void){
  COMMENTDIALOG_open();
}

void openPreferencesDialog(void){
  PREFERENCES_open();
}

void openMIDIPreferencesDialog(void){
  PREFERENCES_open_MIDI();
}

void openToolsDialog(void){
  TOOLS_open();
}

void openMidiLearnPreferencesDialog(void){
  MIDILEARN_PREFS_open();
}

void openAboutWindow(void){
  float length = getSongLength();
  int minutes = length / 60;
  int seconds = length - (minutes*60);
  int s2      = (length - floorf(length)) * 100.0f;
  
  double vblank = GL_get_vblank();
  
  GFX_Message(NULL,
              "<center><b>Radium "  VERSION "</b></center>"
              "<p>"
              "OpenGL vendor: \"%s\"<br>"
              "OpenGL renderer: \"%s\"<br>"
              "OpenGL version: \"%s\"<br>"
              "OpenGL flags: %x<br>"
              "Qt version: \"%s\"<br>"
              "C/C++ compiler version: " __VERSION__ "<br>"
              "S7 version: " S7_VERSION " / " S7_DATE
              "<p>"
              "<A href=\"http://users.notam02.no/~kjetism/radium/development.php\">Credits</A>"
              "<p>"
              "Jack samplerate: %d<br>"
              "Monitor refresh rate: %s"
              "<p>"
              "Song length: %02d : %02d : %02d",
              ATOMIC_GET(GE_vendor_string)==NULL ? "(null)" : ATOMIC_GET(GE_vendor_string),
              ATOMIC_GET(GE_renderer_string)==NULL ? "(null)" : ATOMIC_GET(GE_renderer_string),
              ATOMIC_GET(GE_version_string)==NULL ? "(null)" : ATOMIC_GET(GE_version_string),
              ATOMIC_GET(GE_opengl_version_flags),
              GFX_qVersion(),
              (int)MIXER_get_sample_rate(),
              vblank < 0 ? "Refresh rate not detected" : talloc_format("%.2f", 1000.0 / vblank),
              minutes, seconds, s2
              );
}

char *getProgramPath(void){
  return (char*)OS_get_program_path();
}

char *getConfPath(char *filename){
  return (char*)OS_get_conf_filename2(filename);
}

bool hasConfPath(char *filename){
  return OS_has_conf_filename2(filename);
}

char *getKeybindingsConfPath(void){
  return (char*)OS_get_keybindings_conf_filename2();
}

char *getMenuesConfPath(void){
  return (char*)OS_get_menues_conf_filename2();
}

static const char *g_embedded_audio_files_path = NULL;
const char *getEmbeddedAudioFilesPath(void){
  if (g_embedded_audio_files_path==NULL)
    g_embedded_audio_files_path = SETTINGS_read_string("embedded_audio_files_path", "%home%/.radium/embedded_audio_files");
  
  return g_embedded_audio_files_path;
}

void setEmbeddedAudioFilesPath(const char *new_path){
  g_embedded_audio_files_path = talloc_strdup(new_path);
  SETTINGS_write_string("embedded_audio_files_path", new_path);
}

void save(void){
  Save(root);
}

void saveAs(void){
  SaveAs(root);
}

void saveWithEmbeddedSamples(void){
  SaveWithEmbeddedSamples(root);
}


extern bool isloaded;

void load(void){
  if( Load_CurrPos(getWindowFromNum(-1))){
    isloaded=true;
  }
}

void loadSong(char *filename){
  if( LoadSong_CurrPos(getWindowFromNum(-1),STRING_create(filename))){
    isloaded=true;
  }
}

void newSong(void){
  NewSong_CurrPos(getWindowFromNum(-1));
}

void importMidi(void){
  static bool imported=false;
  if(imported==false){
    PyRun_SimpleString("import import_midi");
    imported=true;
  }
  PyRun_SimpleString("import_midi.import_midi()");
}

static void import_importmod_file(void){
  static bool imported=false;
  if(imported==false){
    PyRun_SimpleString("import import_mod");
    imported=true;
  }else
    PyRun_SimpleString("import_mod=reload(import_mod)"); // Avoid having to restart radium if code is changed. Practical during development. No practical impact on performance either.
  
  GL_create_all(root->song->tracker_windows);
}

void importMod(void){
  //import_importmod_file();
  //PyRun_SimpleString("import_mod.import_mod()");
  //SCHEME_eval("(let () (load \"import_mod.scm\" (curlet)) (load-protracker-module))");
  SCHEME_eval("(my-require 'import_mod.scm)");
  SCHEME_eval("(load-protracker-module)");
  GL_create_all(root->song->tracker_windows);
}

void importXM(void){
  import_importmod_file();
  PyRun_SimpleString("import_mod.import_xm()");
  GL_create_all(root->song->tracker_windows);
}

void insertTracks(int numtracks,int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  InsertTracks_CurrPos(window,(NInt)numtracks);
}

static void insertOrDeleteTrack(int tracknum, int blocknum, int windownum, int num_to_insert){
  struct Tracker_Windows *window=NULL;
  struct WBlocks *wblock;

  wblock=getWBlockFromNumA(
                           windownum,
                           &window,
                           blocknum
                           );

  if(wblock==NULL) return;

  if (tracknum==-1)
    tracknum = wblock->wtrack->l.num;

  ADD_UNDO(Block_CurrPos(window));

  InsertTracks(window,
               wblock,
               tracknum,
               num_to_insert
               );

  window->must_redraw = true;
}

void insertTrack(int tracknum, int blocknum, int windownum){
  insertOrDeleteTrack(tracknum, blocknum, windownum, 1);
}

void deleteTracks(int numtracks,int windownum){
  insertTracks(-numtracks,windownum);
}

void deleteTrack(int tracknum, int blocknum, int windownum){
  insertOrDeleteTrack(tracknum, blocknum, windownum, -1);
}

void deleteBlock(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  DeleteBlock_CurrPos(window);
}

int insertBlock(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return -1;
  return InsertBlock_CurrPos(window)->l.num;
}

int getNumTracks(int blocknum){
  struct WBlocks *wblock = getWBlockFromNum(-1, blocknum);
  if(wblock==NULL) return 0;

  return wblock->block->num_tracks;
}

int getNumLines(int blocknum){
  struct WBlocks *wblock = getWBlockFromNum(-1, blocknum);
  if(wblock==NULL) return 0;

  return wblock->block->num_lines;
}

const char *getBlockName(int blocknum){
  struct WBlocks *wblock = getWBlockFromNum(-1, blocknum);
  if(wblock==NULL) return "";

  return wblock->block->name;
}

int getNumBlocks(void){
  return root->song->num_blocks;
}

void selectBlock(int blocknum, int windownum){
  struct Tracker_Windows *window=NULL;
  struct WBlocks *wblock = getWBlockFromNumA(
                                             windownum,
                                             &window,
                                             blocknum
                                             );
  if(wblock==NULL) return;

  PC_Pause();{
    SelectWBlock(window, wblock);    
  }PC_StopPause(window);
}

void setBlockColor(const_char *colorname, int blocknum, int windownum){
  struct Tracker_Windows *window=NULL;
  struct WBlocks *wblock = getWBlockFromNumA(
                                             windownum,
                                             &window,
                                             blocknum
                                             );
  if(wblock==NULL) return;

  unsigned int color = GFX_get_color_from_colorname(colorname);
  wblock->block->color = color;

  SEQUENCER_update();
}

const char *getBlockColor(int blocknum, int windownum){
  struct Tracker_Windows *window=NULL;
  struct WBlocks *wblock = getWBlockFromNumA(
                                             windownum,
                                             &window,
                                             blocknum
                                             );
  if(wblock==NULL) return "";

  return GFX_get_colorname_from_color(wblock->block->color);
}

void setTrackNoteShowType(int type,int tracknum,int blocknum,int windownum){
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

  wtrack->noteshowtype=type;

#if !USE_OPENGL
  if(window->wblock==wblock){
    DrawUpWTrack(window,wblock,wtrack);
  }
#endif
}

void setTrackVolume(float volume,int tracknum,int blocknum,int windownum){
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

  if(volume<=0.0f)
    wtrack->track->volume = 0;
  else if(volume>=1.0f)
    wtrack->track->volume = MAXTRACKPAN;
  else
    wtrack->track->volume = (int)(volume * (float)MAXTRACKVOL);

  window->must_redraw = true;
}

void setTrackPan(float pan,int tracknum,int blocknum,int windownum){
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

  if(pan<=-1.0f)
    wtrack->track->pan = -MAXTRACKPAN;
  else if(pan>=1.0f)
    wtrack->track->pan = MAXTRACKPAN;
  else
    wtrack->track->pan = (int)(pan * (float)MAXTRACKPAN);

  window->must_redraw = true;
}

void switchTrackNoteShowType(int tracknum,int blocknum,int windownum){
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

  wtrack->noteshowtype++;
  if(wtrack->noteshowtype>MAXTYPE) wtrack->noteshowtype=0;

#if !USE_OPENGL
  if(window->wblock==wblock){
    DrawUpWTrack(window,wblock,wtrack);
  }
#endif
}

void setBlockNoteShowType(int type,int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wblock=getWBlockFromNumA(
	windownum,
	&window,
	blocknum
	);

  if(wblock==NULL) return;

  wtrack=wblock->wtracks;
  while(wtrack!=NULL){
    wtrack->noteshowtype=type;
    wtrack=NextWTrack(wtrack);
  }

#if !USE_OPENGL
  if(window->wblock==wblock){
    DrawUpAllWTracks(window,wblock,NULL);
  }
#endif
}

void switchBlockNoteShowType(int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	0
	);

  if(wtrack==NULL) return;

  int type = wtrack->noteshowtype+1;
  if(type>MAXTYPE)
    type = 0;

  setBlockNoteShowType(type, blocknum, windownum);
}

// track midi channel
int getTrackMidiChannel(int tracknum, int blocknum, int windownum){
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

  if(wtrack==NULL) return 0;

  return ATOMIC_GET(wtrack->track->midi_channel);
}

void setTrackMidiChannel(int channelnum, int tracknum, int blocknum, int windownum){
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

  ADD_UNDO(TrackHeader(window, wblock->block, wtrack->track, wblock->curr_realline));
  ATOMIC_SET(wtrack->track->midi_channel, channelnum);
}


// centtext

bool centtextCanBeTurnedOff(int tracknum, int blocknum, int windownum){
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

  if(wtrack==NULL) return true;

  if (wtrack->notesonoff==0)
    return true;
  
  int num_cents_subtracks = wtrack->centtext_on ? 2 : 0;

  if (WTRACK_num_non_polyphonic_subtracks(wtrack)==num_cents_subtracks)
    return true;
  
  struct Notes *note = wtrack->track->notes;
  while(note!=NULL){
    if (note->note != floorf(note->note))
      return false;
    
    struct Pitches *pitch = note->pitches;
    while(pitch != NULL){
      if (pitch->note != floorf(pitch->note))
        return false;
      pitch = NextPitch(pitch);
    }
    
    note = NextNote(note);
  }

  return true;
}

void showCenttext(bool showit, int tracknum, int blocknum, int windownum){
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

  wtrack->centtext_on = showit;

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

bool centtextVisible(int tracknum,int blocknum,int windownum){
  struct WTracks *wtrack = getWTrackFromNum(-1, blocknum, tracknum);

  if (wtrack==NULL)
    return false;

  return wtrack->centtext_on;
}

void showHideCenttext(int tracknum,int blocknum,int windownum){
  showCenttext(!centtextVisible(tracknum, blocknum, windownum),
              tracknum, blocknum, windownum);
}

void showHideCenttextInBlock(int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	-1
	);

  if(wtrack==NULL) return;

  bool on = !wtrack->centtext_on;

  wtrack = wblock->wtracks;
  while(wtrack!=NULL){
    wtrack->centtext_on = on;
    wtrack = NextWTrack(wtrack);
  }
  
  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

// chancetext

void showChancetext(bool showit, int tracknum, int blocknum, int windownum){
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

  wtrack->chancetext_on = showit;

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

bool chancetextVisible(int tracknum,int blocknum,int windownum){
  struct WTracks *wtrack = getWTrackFromNum(-1, blocknum, tracknum);

  if (wtrack==NULL)
    return false;

  return wtrack->chancetext_on;
}

void showHideChancetext(int tracknum,int blocknum,int windownum){
  showChancetext(!chancetextVisible(tracknum, blocknum, windownum),
              tracknum, blocknum, windownum);
}

void showHideChancetextInBlock(int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	-1
	);

  if(wtrack==NULL) return;

  bool on = !wtrack->chancetext_on;

  wtrack = wblock->wtracks;
  while(wtrack!=NULL){
    wtrack->chancetext_on = on;
    wtrack = NextWTrack(wtrack);
  }
  
  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

// veltext

void showVeltext(bool showit, int tracknum, int blocknum, int windownum){
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

  wtrack->veltext_on = showit;

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

bool veltextVisible(int tracknum,int blocknum,int windownum){
  struct WTracks *wtrack = getWTrackFromNum(-1, blocknum, tracknum);

  if (wtrack==NULL)
    return false;

  return wtrack->veltext_on;
}

void showHideVeltext(int tracknum,int blocknum,int windownum){
  showVeltext(!veltextVisible(tracknum, blocknum, windownum),
              tracknum, blocknum, windownum);
}

void showHideVeltextInBlock(int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	-1
	);

  if(wtrack==NULL) return;

  bool on = !wtrack->veltext_on;

  wtrack = wblock->wtracks;
  while(wtrack!=NULL){
    wtrack->veltext_on = on;
    wtrack = NextWTrack(wtrack);
  }
  
  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

void showFxtext(bool showit, int tracknum, int blocknum, int windownum){
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

  wtrack->fxtext_on = showit;

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

bool fxtextVisible(int tracknum,int blocknum,int windownum){
  struct WTracks *wtrack = getWTrackFromNum(-1, blocknum, tracknum);

  if (wtrack==NULL)
    return false;

  return wtrack->fxtext_on;
}

void showHideFxtext(int tracknum,int blocknum,int windownum){
  showFxtext(!fxtextVisible(tracknum, blocknum, windownum),
              tracknum, blocknum, windownum);
}

void showHideFxtextInBlock(int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	-1
	);

  if(wtrack==NULL) return;

  bool on = !wtrack->fxtext_on;

  wtrack = wblock->wtracks;
  while(wtrack!=NULL){
    wtrack->fxtext_on = on;
    wtrack = NextWTrack(wtrack);
  }
  
  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

void showPianoroll(bool showit, int tracknum,int blocknum,int windownum){
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

  wtrack->pianoroll_on = showit;

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

bool pianorollVisible(int tracknum,int blocknum,int windownum){
  struct WTracks *wtrack = getWTrackFromNum(-1, blocknum, tracknum);

  if (wtrack==NULL)
    return false;

  return wtrack->pianoroll_on;
}

void showHidePianoroll(int tracknum,int blocknum,int windownum){
  showPianoroll(!pianorollVisible(tracknum, blocknum, windownum),
                tracknum, blocknum, windownum);
}

void showHidePianorollInBlock(int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	-1
	);

  if(wtrack==NULL) return;

  bool on = !wtrack->pianoroll_on;

  wtrack = wblock->wtracks;
  while(wtrack!=NULL){
    wtrack->pianoroll_on = on;
    wtrack = NextWTrack(wtrack);
  }
  
  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

void showNoteTrack(bool showit, int tracknum,int blocknum,int windownum){
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

  if (showit)
    wtrack->notesonoff = 1;
  else
    wtrack->notesonoff = 0;
      
  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}


bool noteTrackVisible(int tracknum,int blocknum,int windownum){
  struct WTracks *wtrack = getWTrackFromNum(-1, blocknum, tracknum);

  if (wtrack==NULL)
    return false;

  return wtrack->notesonoff==1;
}


void showHideNoteTrack(int tracknum,int blocknum,int windownum){
  bool is_visible = noteTrackVisible(tracknum, blocknum, windownum);
  showNoteTrack(!is_visible, tracknum, blocknum, windownum);
}

void showHideNoteTracksInBlock(int blocknum,int windownum){
  struct Tracker_Windows *window=NULL;
  struct WTracks *wtrack;
  struct WBlocks *wblock;

  wtrack=getWTrackFromNumA(
	windownum,
	&window,
	blocknum,
	&wblock,
	-1
	);

  if(wtrack==NULL) return;

  int on;
  if (wtrack->notesonoff==0)
    on = 1;
  else
    on = 0;

  wtrack = wblock->wtracks;
  while(wtrack!=NULL){
    wtrack->notesonoff = on;
    wtrack = NextWTrack(wtrack);
  }
  
  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

void showHideSignatureTrack(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;

  window->show_signature_track = !window->show_signature_track;

  if (!window->show_signature_track && window->curr_track==SIGNATURETRACK)
    ATOMIC_WRITE(window->curr_track, 0);

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

void showHideLPBTrack(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;

  window->show_lpb_track = !window->show_lpb_track;

  if (!window->show_lpb_track && window->curr_track==LPBTRACK)
    ATOMIC_WRITE(window->curr_track, 0);

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

void showHideBPMTrack(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;

  window->show_bpm_track = !window->show_bpm_track;

  if (!window->show_bpm_track && window->curr_track==TEMPOTRACK)
    ATOMIC_WRITE(window->curr_track, 0);

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

void showHideReltempoTrack(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;

  window->show_reltempo_track = !window->show_reltempo_track;

  if (!window->show_reltempo_track && window->curr_track==TEMPONODETRACK)
    ATOMIC_WRITE(window->curr_track, 0);

  UpdateAllWBlockCoordinates(window);
  window->must_redraw = true;
}

bool signatureTrackVisible(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return false;
  return window->show_signature_track;
}

bool lpbTrackVisible(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return false;
  return window->show_lpb_track;
}

bool bpmTrackVisible(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return false;
  return window->show_bpm_track;
}

bool reltempoTrackVisible(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return false;
  return window->show_reltempo_track;
}


static bool g_show_linenumbers = false;

bool linenumbersVisible(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_show_linenumbers = SETTINGS_read_bool("show_linenumbers", false);
    has_inited = true;
  }

  return g_show_linenumbers;
}

void setLinenumbersVisible(bool doit){
  g_show_linenumbers = doit;
  SETTINGS_write_bool("show_linenumbers", doit);
  root->song->tracker_windows->must_redraw = true;
}



// Disk

static bool g_stop_playing_when_saving_song = false;

bool doStopPlayingWhenSavingSong(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_stop_playing_when_saving_song = SETTINGS_read_bool("stop_playing_when_saving_song", g_stop_playing_when_saving_song);
    has_inited = true;
  }

  return g_stop_playing_when_saving_song;
}

void setStopPlayingWhenSavingSong(bool val){
  g_stop_playing_when_saving_song = val;
  SETTINGS_write_bool("stop_playing_when_saving_song", val);
}


// audio file save folder

static bool g_save_recorded_audio_files_in_browser_path = false;

bool saveRecordedAudioFilesInBrowserPath(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_save_recorded_audio_files_in_browser_path = SETTINGS_read_bool("save_recorded_audio_files_in_browser_path", false);
    has_inited = true;
  }

  return g_save_recorded_audio_files_in_browser_path;
}

void setSaveRecordedAudioFilesInBrowserPath(bool val){
  if (val != g_save_recorded_audio_files_in_browser_path){
    g_save_recorded_audio_files_in_browser_path = val;
    SETTINGS_write_bool("save_recorded_audio_files_in_browser_path", val);
  }
}

  
// autobackup

static bool g_save_backup_while_playing = true;

bool doSaveBackupWhilePlaying(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_save_backup_while_playing = SETTINGS_read_bool("save_backup_while_playing", true);
    has_inited = true;
  }

  return g_save_backup_while_playing;
}

void setSaveBackupWhilePlaying(bool val){
  g_save_backup_while_playing = val;
  SETTINGS_write_bool("save_backup_while_playing", val);
}


static bool g_do_autobackups = false;

bool doAutoBackups(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_do_autobackups = SETTINGS_read_bool("do_auto_backups", true);
    has_inited = true;
  }

  return g_do_autobackups;
}

void setDoAutoBackups(bool doit){
  g_do_autobackups = doit;
  SETTINGS_write_bool("do_auto_backups", doit);
}

static int g_autobackup_interval_minutes = false;

int autobackupIntervalInMinutes(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_autobackup_interval_minutes = SETTINGS_read_int32("autobackup_interval_minutes", 1);
    has_inited = true;
  }

  return g_autobackup_interval_minutes;
}

void setAutobackupIntervalInMinutes(int interval){
  g_autobackup_interval_minutes = interval;
  SETTINGS_write_int("autobackup_interval_minutes", interval);
}

void addMenuMenu(char* name, char* command){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  GFX_AddMenuMenu(window, name, command);
}

void goPreviousMenuLevel(void){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  GFX_GoPreviousMenuLevel(window);
}

void addMenuItem(char* name, char* command){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  GFX_AddMenuItem(window, name, command);
}

void addCheckableMenuItem(char* name, char* command, int checkval){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  GFX_AddCheckableMenuItem(window, name, command, checkval==1?true:false);
}

void addMenuSeparator(void){
  struct Tracker_Windows *window=getWindowFromNum(-1);if(window==NULL) return;
  GFX_AddMenuSeparator(window);
}

void setStatusbarText(char* text, int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);if(window==NULL) return;
  static bool is_empty = false;
  if (text[0]=='\0') {
    if (!is_empty) {
      GFX_SetStatusBar(window,text);
      is_empty = true;
    }
    
  } else {

    GFX_SetStatusBar(window,text);
    is_empty = false;
    
  }
  
}

int getWebserverPort(void){
  return SCHEME_get_webserver_port();
}

void eraseEstimatedVblank(void){
#if USE_QT5
  printf("eraseEstimatedVblank() is not used in Qt5\n");
#else
  GL_erase_estimated_vblank();
#endif
}

void evalScheme(const_char *code){
  SCHEME_eval(code);
}

void evalPython(const_char *code){
  PyRun_SimpleString(code);
}

bool isFullVersion(void){
  return FULL_VERSION==1;
}


static bool g_vst_gui_always_on_top;

bool vstGuiIsAlwaysOnTop(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_vst_gui_always_on_top = SETTINGS_read_bool("vst_gui_always_on_top", true);
    has_inited = true;
  }

  return g_vst_gui_always_on_top;
}

void setVstGuiAlwaysOnTop(bool doit){
  if (doit != g_vst_gui_always_on_top) {
    g_vst_gui_always_on_top = doit;
    SETTINGS_write_bool("vst_gui_always_on_top", doit);
    PREFERENCES_update();
  }
}


static bool g_modal_windows;

bool doModalWindows(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_modal_windows = SETTINGS_read_bool("modal_windows", GL_should_do_modal_windows());
    has_inited = true;
  }

  return g_modal_windows;
}

void setModalWindows(bool doit){
  g_modal_windows = doit;
  SETTINGS_write_bool("modal_windows", doit);
}

//

static DEFINE_ATOMIC(int, g_high_cpu_protection_opengl_protection) = -1;

bool doHighCpuOpenGlProtection(void){  
  int g = ATOMIC_GET(g_high_cpu_protection_opengl_protection);
  
  if (g == -1){
    g = SETTINGS_read_bool("high_cpu_protection_opengl_protection", true) ? 1 : 0;
    ATOMIC_SET(g_high_cpu_protection_opengl_protection, g);
  }

  return g==1 ? true : false;
}

void setHighCpuOpenGlProtection(bool doit){
  ATOMIC_SET(g_high_cpu_protection_opengl_protection, doit ? 1 : 0);
  SETTINGS_write_bool("high_cpu_protection_opengl_protection", doit);
}

//

static DEFINE_ATOMIC(int, g_lock_juce_when_swapping_opengl) = -1;

bool doLockJuceWhenSwappingOpenGL(void){
  int g = ATOMIC_GET(g_lock_juce_when_swapping_opengl);
  
  if (g == -1){
    g = bool_to_int(SETTINGS_read_bool("lock_juce_when_swapping_opengl", false));

    ATOMIC_SET(g_lock_juce_when_swapping_opengl, g);
    if (g==1){
      if (SETTINGS_read_bool("show_lock_juce_when_swapping_opengl_warning", true)) {
        vector_t v = {};
        VECTOR_push_back(&v,"OK");
        int dont_show = VECTOR_push_back(&v,"Don't show this message again.");

        int result = GFX_Message(&v, "Warning: the \"Don't run plugin GUI code when swapping OpenGL\" option is probably not necessary anymore.\n\nIt might also reduce graphical performance.");

        if (result==dont_show)
          SETTINGS_write_bool("show_lock_juce_when_swapping_opengl_warning", false);
      }
    }
  }

  return int_to_bool(g);
}

void setLockJuceWhenSwappingOpenGL(bool doit){
  ATOMIC_SET(g_lock_juce_when_swapping_opengl, bool_to_int(doit));
  SETTINGS_write_bool("lock_juce_when_swapping_opengl", doit);
}



static bool g_native_file_requesters;

bool useNativeFileRequesters(void){
#if FOR_WINDOWS

  return true;  // Workaround. non-native QFileDialog freezes on windows.

#else

  static bool has_inited = false;

  if (has_inited==false){
    float default_value = false;
    g_native_file_requesters = SETTINGS_read_bool("native_file_requesters", default_value);
    has_inited = true;
  }

  return g_native_file_requesters;
#endif
}

void setUseNativeFileRequesters(bool doit){
  g_native_file_requesters = doit;
  SETTINGS_write_bool("native_file_requesters", doit);
}


static DEFINE_ATOMIC(bool, g_always_run_buses) = false;
bool doAlwaysRunBuses(void){
  static bool has_inited = false;

  if (has_inited==false){
    ATOMIC_SET(g_always_run_buses, SETTINGS_read_bool("always_run_buses", false));
    has_inited=true;
  }

  return ATOMIC_GET(g_always_run_buses);
}
void setAlwaysRunBuses(bool doit){
  printf("Setting always stuff to %d\n",doit);
  ATOMIC_SET(g_always_run_buses, doit);
  SETTINGS_write_bool("always_run_buses", doit);
}


void printMixerTree(void){
  SP_print_tree();
}

void testCrashreporter(void){
  //R_ASSERT(false);
  //return;
#if !defined(RELEASE)
  abort(); // The crash below usually doesn't work in non-release mode since we usually compile with fsanitize=address
#endif
  int *ai=NULL;
  ai[0] = 50;
}

extern bool g_test_crashreporter_in_audio_thread;
void testCrashreporterInAudioThread(void){
  g_test_crashreporter_in_audio_thread = true;
}

void testErrorMessage(void){
  SYSTEM_show_message("Error message seems to work");
}

// FAUST

static int g_faust_optimization_level;

int getFaustOptimizationLevel(void){
  static bool has_inited = false;

  if (has_inited==false){
    int default_value = 4;
    g_faust_optimization_level = SETTINGS_read_int32("faust_optimization_level", default_value);
    has_inited = true;
  }

  return g_faust_optimization_level;
}

void setFaustOptimizationLevel(int level){
  g_faust_optimization_level = level;
  SETTINGS_write_int("faust_optimization_level", level);
}


static const char *g_faust_gui_style;

const char *getFaustGuiStyle(void){
  static bool has_inited = false;

  if (has_inited==false){
    const char *default_value = "Blue";
    g_faust_gui_style = SETTINGS_read_string("faust_gui_style", default_value);
    has_inited = true;
  }

  return g_faust_gui_style;
}

void setFaustGuiStyle(const char *style){
  g_faust_gui_style = talloc_strdup(style);
  SETTINGS_write_string("faust_gui_style", style);
  FAUST_change_qtguistyle(style);
}


// PLAYLIST

/*
void setPlaylistLength(int len){
  BL_setLength(len);
}

void setPlaylistBlock(int pos, int blocknum){
  struct Blocks *block = getBlockFromNum(blocknum);
  if (block==NULL)
    return;

  BL_setBlock(pos, block);
}
*/

static double get_block_length(struct Blocks *block){
  double time = getBlockSTimeLength(block);

  time /= ATOMIC_DOUBLE_GET(block->reltempo);

  return time / (double)MIXER_get_sample_rate();
}

float getBlockLength(int blocknum, int windownum){
  struct WBlocks *wblock = getWBlockFromNum(windownum, blocknum);
  if(wblock==NULL) return 1.0; // return 1.0 instead of 0.0 to avoid divide by zero errors.

  return get_block_length(wblock->block);
}

float getSongLength(void){
  return SONG_get_length();
}

int64_t getSongLengthInFrames(void){
  return SONG_get_length() * MIXER_get_sample_rate();
}

int getSampleRate(void){
  return MIXER_get_sample_rate();
}

int getLogtypeHold(void){
  return LOGTYPE_HOLD;
}

Place getCursorPlace(int blocknum, int windownum){
  struct WBlocks *wblock = getWBlockFromNum(windownum, blocknum);if(wblock==NULL) return place(0,0,1);

  return wblock->reallines[wblock->curr_realline]->l.p;
}

int getCursorTrack(int windownum){
  struct Tracker_Windows *window=getWindowFromNum(windownum);

  return window->curr_track;
}

int getHighestLegalPlaceDenominator(void){
  return MAX_UINT32;
}

char *toBase64(const char *s){
  return STRING_get_chars(STRING_toBase64(STRING_create(s)));
}

char *fromBase64(const char *s){
  return STRING_get_chars(STRING_fromBase64(STRING_create(s)));
}

void msleep(int ms){
  usleep(1000*ms);
}



// Scheduler
////////////////////////////

#define MAX_SCHEDULED_CALLBACKS 1024

namespace{
  struct ScheduledEvent{
    ScheduledEvent *next = NULL;

    double priority = 0.0;
    func_t *_callback = NULL;

    ScheduledEvent()
    {
    }

    void call_before_usage(double daspriority, func_t *callback){
      R_ASSERT(_callback==NULL);
      priority = daspriority;
      _callback = callback;
      s7extra_protect(_callback);
    }

    void call_after_usage(void){
      s7extra_unprotect(_callback);
      _callback = NULL;
    }
  };
}

static ScheduledEvent *g_unused_events = NULL;
static radium::PriorityQueue<ScheduledEvent> g_scheduled_events(MAX_SCHEDULED_CALLBACKS);

static void release_event(ScheduledEvent *event){
  R_ASSERT(event->next==NULL);

  event->call_after_usage();
  event->next = g_unused_events;
  g_unused_events = event;
}

static void schedule(ScheduledEvent *event){
  if (g_scheduled_events.add(event)==false){
    handleError("Can not schedule event. Queue is full.");
    release_event(event);
  }
}

void schedule(double ms, func_t *callback){
  ScheduledEvent *event;

  if (g_unused_events!=NULL){
    event = g_unused_events;
    g_unused_events = event->next;
    event->next = NULL;
  } else
    event = new ScheduledEvent();

  double priority = TIME_get_ms() + ms;

  event->call_before_usage(priority, callback);

  schedule(event);
}

void API_call_very_often(void){
  double time = TIME_get_ms();

  while(true){
    ScheduledEvent *event = g_scheduled_events.get_first_event();
    if (event==NULL)
      break;

    if (event->priority > time)
      break;

    g_scheduled_events.remove_first_event();

    dyn_t ret = s7extra_callFunc_dyn_void(event->_callback);
    double new_ms = 0;
    
    if (ret.type==INT_TYPE)
      new_ms = ret.int_number;
    
    else if (ret.type==FLOAT_TYPE)
      new_ms = ret.float_number;

    if (new_ms > 0){
      event->priority = time + new_ms;
      schedule(event);
    } else {
      release_event(event);
    }
  }
}

