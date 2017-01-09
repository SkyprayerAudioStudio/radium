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

#include "Python.h"

#include "../common/nsmtracker.h"
#include "../common/list_proc.h"
#include "../common/placement_proc.h"
#include "../common/velocities_proc.h"
#include "../common/windows_proc.h"
#include "../common/wtracks_proc.h"
#include "../common/tracks_proc.h"
#include "../common/notes_proc.h"
#include "../common/tempos_proc.h"
#include "../common/LPB_proc.h"
#include "../common/Signature_proc.h"
#include "../common/undo_signatures_proc.h"
#include "../common/undo_lpbs_proc.h"
#include "../common/undo_tempos_proc.h"
#include "../common/time_proc.h"
#include "../common/player_proc.h"
#include "../common/player_pause_proc.h"
#include "../common/undo_maintempos_proc.h"
#include "../common/Beats_proc.h"
#include "../common/wblocks_proc.h"
#include "../common/settings_proc.h"
#include "../common/undo_notes_proc.h"
#include "../common/visual_proc.h"

#include "api_common_proc.h"
#include "api_support_proc.h"
#include "radium_proc.h"



extern char *NotesTexts2[];
extern char *NotesTexts3[];


char *getNoteName2(int notenum){
  if (notenum<0 || notenum>127)
    return "";
  else
    return NotesTexts2[notenum];
}

char *getNoteName3(int notenum){
  if (notenum<0 || notenum>127)
    return "";
  else
    return NotesTexts3[notenum];
}

float getNoteNameValue(char *notename){
  return notenum_from_notetext(notename);
}

int getPianorollLowKey(int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);
  if (wtrack==NULL)
    return 0;
  return wtrack->pianoroll_lowkey;
}

int getPianorollHighKey(int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);
  if (wtrack==NULL)
    return 127;
  return wtrack->pianoroll_highkey;
}

void setPianorollLowKey(int key, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);
  if (wtrack==NULL)
    return;

  int newkey = R_BOUNDARIES(0, key, 127);

  if (wtrack->pianoroll_highkey - newkey < 5)
    return;
  
  wtrack->pianoroll_lowkey = newkey;
  
  UpdateWBlockCoordinates(window,wblock);
  window->must_redraw=true;
}

void setPianorollHighKey(int key, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);
  if (wtrack==NULL)
    return;

  int newkey = R_BOUNDARIES(0, key, 127);

  if (newkey - wtrack->pianoroll_lowkey < 5)
    return;
  
  wtrack->pianoroll_highkey = newkey;

  UpdateWBlockCoordinates(window,wblock);
  window->must_redraw=true;
}

float getLowestKey(int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);
  if (wtrack==NULL)
    return -1;

  return TRACK_get_min_pitch(wtrack->track);
}

float getHighestKey(int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);
  if (wtrack==NULL)
    return -1;

  return TRACK_get_max_pitch(wtrack->track);
}


void incNoteVolume(int incvolume,int windownum){
	struct Tracker_Windows *window=getWindowFromNum(windownum);
	if(window==NULL) return;

	IncreaseVelocityCurrPos(window,incvolume);
}

static bool g_scrollplay = false;

bool doScrollPlay(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_scrollplay = SETTINGS_read_bool("scroll_play", true);
    has_inited = true;
  }

  return g_scrollplay;
}

void setScrollPlay(bool doit){
  g_scrollplay = doit;
  SETTINGS_write_bool("scroll_play", doit);
}
                          
void switchScrollPlayOnOff(void){
  setScrollPlay(!doScrollPlay());
}



static bool g_do_scroll_edit_lines = false;

bool doScrollEditLines(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_do_scroll_edit_lines = SETTINGS_read_bool("arrow_keys_scroll_edit_lines", false);
    has_inited = true;
  }

  return g_do_scroll_edit_lines;
}

void setScrollEditLines(bool doit){
  g_do_scroll_edit_lines = doit;
  SETTINGS_write_bool("arrow_keys_scroll_edit_lines", doit);
}


static bool g_do_autorepeat = false;

bool doAutoRepeat(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_do_autorepeat = SETTINGS_read_bool("keyboard_autorepeat", false);
    has_inited = true;
  }

  return g_do_autorepeat;
}

void setAutoRepeat(bool doit){
  g_do_autorepeat = doit;
  SETTINGS_write_bool("keyboard_autorepeat", doit);
}


static bool g_do_range_paste_cut = true;

bool doRangePasteCut(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_do_range_paste_cut = SETTINGS_read_bool("range_paste_cut", true);
    has_inited = true;
  }

  return g_do_range_paste_cut;
}

void setRangePasteCut(bool doit){
  g_do_range_paste_cut = doit;
  SETTINGS_write_bool("range_paste_cut", doit);
}


static bool g_do_range_paste_scroll_down = true;

bool doRangePasteScrollDown(void){
  static bool has_inited = false;

  if (has_inited==false){
    g_do_range_paste_scroll_down = SETTINGS_read_bool("range_paste_scroll_down", true);
    has_inited = true;
  }

  return g_do_range_paste_scroll_down;
}

void setRangePasteScrollDown(bool doit){
  g_do_range_paste_scroll_down = doit;
  SETTINGS_write_bool("range_paste_scroll_down", doit);
}



int g_downscroll = 1;

void setNoteScrollLength(int l){
  R_ASSERT_RETURN_IF_FALSE(l>=0);

  if (l != g_downscroll){
  
    g_downscroll = l;

    GFX_OS_update_bottombar();
  }
}

int getNoteScrollLength(void){
  return g_downscroll;
}



/********** Signatures  **********/

void setMainSignature(int numerator, int denominator){
  if (numerator<=0 || denominator<=0)
    return;
  if (numerator==root->signature.numerator && denominator==root->signature.denominator)
    return;
  
  struct Tracker_Windows *window = root->song->tracker_windows;
  struct WBlocks *wblock = window->wblock;

  ADD_UNDO(MainTempo(window,wblock));

  PC_Pause();{
    root->signature = ratio(numerator, denominator);
    UpdateAllBeats();
  }PC_StopPause(window);
  
  window->must_redraw = true;
}

Place getMainSignature(void){
  return place(0,root->signature.numerator,root->signature.denominator);
}

int numSignatures(int blocknum, int windownum){
  struct WBlocks *wblock=getWBlockFromNum(windownum,blocknum);
  if(wblock==NULL)
    return 0;

  return ListFindNumElements3(&wblock->block->signatures->l);
}

int addSignature(int numerator, int denominator,
                 Place place,
                 int blocknum)
{
  struct Tracker_Windows *window;
  struct WBlocks *wblock=getWBlockFromNumA(-1,&window,blocknum);
  if(wblock==NULL) {
    handleError("unknown block(%p)",blocknum);
    return -1;
  }

  if (!PlaceLegal(wblock->block, &place)) {
    handleError("Place %s is not legal", PlaceToString(&place));
    return -1;
  }

  ADD_UNDO(Signatures_CurrPos(window));
        
  struct Signatures *signature = SetSignature(wblock->block,&place,ratio(numerator, denominator));

  window->must_redraw=true;

  return ListFindElementPos3(&wblock->block->signatures->l,&signature->l);
}

int addSignature3(int numerator, int denominator,
                  int line,int counter,int dividor,
                  int blocknum)
{
  Place place = {line, counter, dividor};
  return addSignature(numerator, denominator, place, blocknum);
}

// Return a place, for convenience. The line part of the returned value is always 0 (even if numerator > denominator). Returns -1 on error.
Place getSignature(int signaturenum, int blocknum, int windownum){
  struct Signatures *signature = getSignatureFromNum(windownum, blocknum, signaturenum);
  if (signature==NULL)
    return place(0,-1,1);
  else
    return place(0, signature->signature.numerator, signature->signature.denominator);
}

/******************* LPBs *************************/

void setMainLPB(int lpb_value){
  if (lpb_value <=1)
    return;
  if (lpb_value == root->lpb)
    return;
  
  struct Tracker_Windows *window = root->song->tracker_windows;
  struct WBlocks *wblock = window->wblock;

  printf("Undo MainTempo lpb: %d\n",lpb_value);
  ADD_UNDO(MainTempo(window,wblock));

  PC_Pause();{
    root->lpb=lpb_value;
    UpdateAllSTimes();
    UpdateAllBeats();
  }PC_StopPause(window);
  
  //UpdateAllWLPBs(window);
  window->must_redraw = true;
}

int getMainLPB(void){
  return root->lpb;
}

int numLPBs(int blocknum, int windownum){
  struct WBlocks *wblock=getWBlockFromNum(windownum,blocknum);
  if(wblock==NULL)
    return 0;

  return ListFindNumElements3(&wblock->block->lpbs->l);
}

int addLPB(int lpb_value,
           Place place,
           int blocknum)
{
  struct Tracker_Windows *window;
  struct WBlocks *wblock=getWBlockFromNumA(-1,&window,blocknum);
  if(wblock==NULL)
    return -1;

  if (!PlaceLegal(wblock->block, &place)) {
    handleError("Place %s is not legal", PlaceToString(&place));
    return -1;
  }

  ADD_UNDO(LPBs_CurrPos(window));
  
  struct LPBs *lpb = SetLPB(wblock->block,&place,lpb_value);

  window->must_redraw=true;

  return ListFindElementPos3(&wblock->block->lpbs->l,&lpb->l);
}

int addLPB3(int lpb,
            int line,int counter,int dividor,
            int blocknum
            )
{
  Place place = {line, counter, dividor};

  return addLPB(lpb, place, blocknum);
}

int getLPB(int num, int blocknum, int windownum){
  struct LPBs *lpb = getLPBFromNum(windownum, blocknum, num);
  if (lpb==NULL)
    return -1;
  else
    return lpb->lpb;
}


/***************** BPMs *************************/

void setMainBPM(int bpm_value){
  if (bpm_value <=1)
    return;
  if (bpm_value == root->tempo)
    return;

  struct Tracker_Windows *window = root->song->tracker_windows;
  struct WBlocks *wblock = window->wblock;
  
  ADD_UNDO(MainTempo(window,wblock));

  PC_Pause();{
    root->tempo=bpm_value;
    UpdateAllSTimes();
    R_ASSERT(wblock->block->beats != NULL);
  }PC_StopPause(window);
}

int getMainBPM(void){
  return root->tempo;
}

int numBPMs(int blocknum, int windownum){
  struct WBlocks *wblock=getWBlockFromNum(windownum,blocknum);
  if(wblock==NULL)
    return 0;

  return ListFindNumElements3(&wblock->block->tempos->l);
}


int addBPM(int bpm,
           Place place,
           int blocknum)
{
  struct Tracker_Windows *window;
  struct WBlocks *wblock=getWBlockFromNumA(-1,&window,blocknum);
  if(wblock==NULL)
    return -1;

  if (!PlaceLegal(wblock->block, &place)) {
    handleError("Place %s is not legal", PlaceToString(&place));
    return -1;
  }

  ADD_UNDO(Tempos_CurrPos(window));

  struct Tempos *tempo = SetTempo(wblock->block,&place,bpm);

  window->must_redraw=true;

  return ListFindElementPos3(&wblock->block->tempos->l,&tempo->l);
}

int addBPM3(int bpm,
            int line,int counter,int dividor,
            int blocknum
            )
{
  Place place = {line, counter, dividor};

  return addBPM(bpm, place, blocknum);
}
           
int getBPM(int num, int blocknum, int windownum){
  struct BPMs *bpm = getBPMFromNum(windownum, blocknum, num);
  if (bpm==NULL)
    return -1;
  else
    return bpm->tempo;
}

Place getBPMPlace(int num, int blocknum, int windownum){
  struct BPMs *bpm = getBPMFromNum(windownum, blocknum, num);
  if (bpm==NULL)
    return place(-1,0,1);
  else
    return bpm->l.p;
}


/****************** notes **********************/

int getNoteVolume(int windownum,int blocknum,int tracknum,int notenum){
	struct Notes *note=getNoteFromNum(windownum,blocknum,tracknum,notenum);

	if(note==NULL) return -1;

  return note->velocity;
}

int getNumNotes(int tracknum,int blocknum,int windownum){
	struct WTracks *wtrack=getWTrackFromNum(windownum,blocknum,tracknum);

	if(wtrack==NULL) return 0;
	if(wtrack->track->notes==NULL) return 0;

	return ListFindNumElements3(&wtrack->track->notes->l);
}

bool noteContinuesNextBlock(int notenum, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack;
  struct Notes *note = getNoteFromNumA(windownum, &window, blocknum, &wblock, tracknum, &wtrack, notenum);
  if (note==NULL)
    return false;

  return note_continues_next_block(wblock->block, note);
}

void setNoteContinueNextBlock(bool continuenextblock, int notenum, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack;
  struct Notes *note = getNoteFromNumA(windownum, &window, blocknum, &wblock, tracknum, &wtrack, notenum);
  if (note==NULL)
    return;
  
  ADD_UNDO(Notes(window,
                 wblock->block,
                 wtrack->track,
                 wblock->curr_realline
                 )
           );

  note->noend = continuenextblock?1:0;
}

int addNote(float notevalue,
            float velocity,
            Place start,
            Place end,
            int tracknum, int blocknum, int windownum)
{
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);

  if(wtrack==NULL)
    return -1;

  if (notevalue < 0.001){
    handleError("addNote: Pitch less than 0.001: %f\n", notevalue);
    return -1;
  }
  
  if (start.line==0 && start.counter==1 && start.dividor==MAX_UINT32)
    start.counter = 0;
      
  ValidatePlace(&start);

        
  if (!PlaceLegal(wblock->block, &start)) {
    handleError("addNote: Start place %d + %d/%d is not legal", start.line, start.counter, start.dividor);
    return -1;
  }

  Place *end_place = NULL;

  if (end.line >=0) {
    end_place = &end;
    
    ValidatePlace(end_place);

    if (end.line==wblock->block->num_lines && end.counter==0)
      PlaceSetLastPos(wblock->block, end_place);
    
    if (!PlaceLegal(wblock->block, end_place)) {
      handleError("addNote: End place %d + %d/%d is not legal", end.line, end.counter, end.dividor);
      return -1;
    }

    if (PlaceLessOrEqual(end_place, &start)){
      handleError("addNote: Note end (%s) positioned before or at note start (%s)", PlaceToString(end_place), PlaceToString(&start));
      return -1;
    }
  }

  struct Notes *note = InsertNote(wblock,
                                  wtrack,
                                  &start,
                                  end_place,
                                  notevalue,
                                  MAX_VELOCITY*velocity,
                                  true);

  window->must_redraw=true;

  return ListFindElementPos3(&wtrack->track->notes->l,&note->l);
}

int addNote3(float notevalue,float velocity,
             int line,int counter,int dividor,
             int end_line,int end_counter,int end_dividor,
             int tracknum, int blocknum, int windownum)
{
  Place start = place(line,counter,dividor);

  Place end = place(end_line, end_counter, end_dividor);

  return addNote(notevalue, velocity, start, end, tracknum, blocknum, windownum);
}

void cutNote(Place place, int notenum, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack;
  struct Notes *note = getNoteFromNumA(windownum, &window, blocknum, &wblock, tracknum, &wtrack, notenum);
  if (note==NULL)
    return;

  if (PlaceGreaterOrEqual(&place, &note->end))
    return;
  
  if (PlaceLessOrEqual(&place, &note->l.p))
    return;

  PLAYER_lock();{
    CutNoteAt(wblock->block, wtrack->track, note, &place);
  }PLAYER_unlock();
  
}

void deleteNote(int notenum, int tracknum, int blocknum, int windownum){
  deletePianonote(0, notenum, tracknum, blocknum, windownum);
}

void deleteAllNotesInTrack(int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(windownum, &window, blocknum, &wblock, tracknum);
  if (wtrack==NULL)
    return;

  wtrack->track->notes = NULL;
  
  window->must_redraw=true;
}

// notes
//////////////////////////////////////////////////

void undoNotes(int tracknum, int blocknum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack = getWTrackFromNumA(-1, &window, blocknum, &wblock, tracknum);
  if(wtrack==NULL)
    return;
  ADD_UNDO(Notes(window,window->wblock->block,wtrack->track,window->wblock->curr_realline));
}

Place getNoteStart(int notenum, int tracknum, int blocknum, int windownum){
  struct Notes *note=getNoteFromNum(windownum,blocknum,tracknum,notenum);

  if(note==NULL)
    return place(0,0,1);

  return note->l.p;
}

Place getNoteEnd(int notenum, int tracknum, int blocknum, int windownum){
  struct Notes *note=getNoteFromNum(windownum,blocknum,tracknum,notenum);

  if(note==NULL)
    return place(1,0,1);

  return note->end;
}

float getNoteValue(int notenum, int tracknum, int blocknum, int windownum){
  struct Notes *note=getNoteFromNum(windownum,blocknum,tracknum,notenum);

  if(note==NULL)
    return 64.0f;

  return note->note;
}

float getNoteEndPitch(int notenum, int tracknum, int blocknum, int windownum){
  struct Notes *note=getNoteFromNum(windownum,blocknum,tracknum,notenum);

  if(note==NULL)
    return 0;

  return note->pitch_end;
}

int getNoteSubtrack(int notenum, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack;
  struct Notes *note = getNoteFromNumA(windownum, &window, blocknum, &wblock, tracknum, &wtrack, notenum);
  if (note==NULL)
    return 0;

  return GetNoteSubtrack(wtrack, note);
}

void setNoMouseNote(int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock = getWBlockFromNumA(windownum, &window, blocknum);
  if(wblock==NULL)
    return;
  
  if (wblock->mouse_note != NULL){
    wblock->mouse_note = NULL;
    window->must_redraw_editor = true;
    //printf("no mouse note dirty\n");
  }
}

void setMouseNote(int notenum, int tracknum, int blocknum, int windownum){
  struct Tracker_Windows *window;
  struct WBlocks *wblock;
  struct WTracks *wtrack;
  struct Notes *note = getNoteFromNumA(windownum, &window, blocknum, &wblock, tracknum, &wtrack, notenum);
  if (note==NULL)
    return;
  else if (wblock->mouse_note != note){
    wblock->mouse_note = note;
    window->must_redraw_editor = true;
    //printf("mouse note dirty\n");
  }
}

