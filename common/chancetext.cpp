/* Copyright 2016 Kjetil S. Matheussen

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



#include <math.h>

#include "nsmtracker.h"
#include "vector_proc.h"
#include "realline_calc_proc.h"
#include "undo.h"
#include "undo_notes_proc.h"
#include "trackreallines2_proc.h"
#include "data_as_text_proc.h"
#include "atomic.h"

#include "chancetext_proc.h"

int CHANCETEXT_subsubtrack(struct Tracker_Windows *window, struct WTracks *wtrack){
  int curr_track_sub = window->curr_track_sub;
  if (wtrack->chancetext_on == false)
    return -1;

  if (wtrack->centtext_on)
    curr_track_sub -= 2;

  if (curr_track_sub < 0)
    return -1;

  if (curr_track_sub >= 2)
    return -1;

  return curr_track_sub;
}

bool CHANCETEXT_keypress(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, int realline, Place *place, int key){
  int subsubtrack = CHANCETEXT_subsubtrack(window, wtrack);

  if (subsubtrack==-1)
    return false;
  
  const Trs &trs = TRS_get(wblock, wtrack, realline);

  ADD_UNDO(Notes_CurrPos(window));  

  if (trs.size() > 1) {

    // MORE THAN ONE ELEMENT
    
    if (key == EVENT_DEL){

      for (const TrackRealline2 &tr2 : trs){
        struct Notes *note = tr2.note;
        struct Pitches *pitch = tr2.pitch;
        if (pitch!=NULL)
          safe_int_write(&pitch->chance, 0x100);
        else if (note!=NULL)
          safe_int_write(&note->chance, 0x100);
      }
      
    } else {
      
      Undo_CancelLastUndo();
      
    }
    
  } else if (trs.size() == 0) {

    // NO ELEMENTS
    
    Undo_CancelLastUndo();

  } else {

    // ONE ELEMENT
    
    const TrackRealline2 &tr2 = trs[0];
    struct Notes *dasnote = tr2.note;
    struct Pitches *pitch = tr2.pitch;
  
    if (key == EVENT_DEL) {

      if (pitch!=NULL)
        safe_int_write(&pitch->chance, 0x100);
      else if (pitch!=NULL)
        safe_int_write(&dasnote->chance, 0x100);
      else
        return false;
      
    } else {

      int chance;
      
      if (pitch!=NULL)
        chance = pitch->chance;
      else if (dasnote!=NULL)
        chance = dasnote->chance;
      else
        return false;
      
      data_as_text_t dat = DAT_get_overwrite(chance, 0, subsubtrack, key, 0, 0xff, 0, 0xff, true, false);

      if (dat.is_valid==false)
        return false;

      //double new_note = floor(note) + ((double)dat.value / 100.0);
      printf("new_chance: %d\n",dat.value);
      
      if (pitch!=NULL)
        safe_int_write(&pitch->chance, dat.value);
      else
        safe_int_write(&dasnote->chance, dat.value);
      
    }    
  }

  return true;
}
  
