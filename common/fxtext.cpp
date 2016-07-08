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
#include "list_proc.h"
#include "realline_calc_proc.h"
#include "undo_fxs_proc.h"
#include "undo.h"
#include "data_as_text_proc.h"
#include "fxlines_proc.h"

#include "fxtext_proc.h"


static void add_fxtext(const struct WBlocks *wblock, FXText_trss &fxtexts, const struct FX *fx, struct FXNodeLines *fxnodeline){

  int realline = FindRealLineFor(wblock, 0, &fxnodeline->l.p);      
  FXText_trs &v = fxtexts[realline];

  FXText fxtext = {};

  fxtext.p = fxnodeline->l.p;
  fxtext.fx = fx;
  fxtext.fxnodeline = fxnodeline;
  
  fxtext.value = round(scale_double(fxnodeline->val, fx->min, fx->max, 0, 0x100));
  if (fxtext.value==0x100)
    fxtext.value=0xff;
  fxtext.logtype = fxnodeline->logtype;

  TRS_INSERT_PLACE(v, fxtext);
}


// Returns a pointer to AN ARRAY of vectors (one vector for each realline), not a pointer to a vector (as one would think).
const FXText_trss FXTEXTS_get(const struct WBlocks *wblock, const struct WTracks *wtrack, const struct FXs *fxs){
  FXText_trss fxtexts;

  struct FXNodeLines *fxnodeline = fxs->fxnodelines;
  while(fxnodeline!=NULL){
    add_fxtext(wblock, fxtexts, fxs->fx, fxnodeline);
    fxnodeline = NextFXNodeLine(fxnodeline);
  }

  return fxtexts;
}

int FXTEXT_subsubtrack(const struct Tracker_Windows *window, struct WTracks *wtrack, struct FXs **to_fxs){
  if (wtrack->fxtext_on == false)
    return -1;

  int subsubtrack = window->curr_track_sub;

  if (wtrack->centtext_on == true)
    subsubtrack -= 2;
  
  if (wtrack->chancetext_on == true)
    subsubtrack -= 2;
  
  if (wtrack->veltext_on == true)
    subsubtrack -= 3;

  if (subsubtrack < 0)
    return -1;

  VECTOR_FOR_EACH(struct FXs *, fxs, &wtrack->track->fxs){
    if (subsubtrack == 0 || subsubtrack == 1 || subsubtrack == 2){
      if (to_fxs!=NULL)
        *to_fxs = fxs;
      return subsubtrack;
    }
    
    subsubtrack -= 3;
  }END_VECTOR_FOR_EACH;
  
  return -1;
}

bool FXTEXT_keypress(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, int realline, Place *place, int key){
  struct FXs *fxs;
  
  int subsubtrack = FXTEXT_subsubtrack(window, wtrack, &fxs);
  if (subsubtrack==-1)
    return false;

  struct FX *fx = fxs->fx;
  //printf("FXmin: %d, fxmax: %d\n",fx->min, fx->max);
  
  const FXText_trss &fxtexts = FXTEXTS_get(wblock, wtrack, fxs);

  const FXText_trs &fxtext = fxtexts[realline];

  //  if (fxtext->num_elements == 0 && val==0)
  //   return true;

  ADD_UNDO(FXs(window, wblock->block, wtrack->track, wblock->curr_realline));

  if (fxtext.size() > 1){

    // MORE THAN ONE ELEMENT
    
    if (key == EVENT_DEL){
      
      for (auto vt : fxtext) {
        if (VECTOR_is_in_vector(&wtrack->track->fxs, fxs) && isInList3(&fxs->fxnodelines->l, &vt.fxnodeline->l)) // We might have removed all nodes already (see line below)
          DeleteFxNodeLine(window, wtrack, fxs, vt.fxnodeline); // In this line, two nodes are removed if there's only two left.
      }
      
    } else {
      
      Undo_CancelLastUndo();
      
    }
    
  } else if (fxtext.size() == 0){

    // NO ELEMENTS

    if (fx == NULL){

      fprintf(stderr, "Can this happen?\n");
      Undo_CancelLastUndo();
      
    } else {

      data_as_text_t dat = DAT_get_newvalue(subsubtrack, key, round(scale_double(0x80, 0, 0x100, fx->min, fx->max)), fx->min, fx->max, true);
      if (dat.value > fx->max)
        dat.value = fx->max;
      
      if (dat.is_valid==false)
        return false;

      int pos = AddFXNodeLine(window, wblock, wtrack, fxs, dat.value, place);
            
      struct FXNodeLines *fxnodeline = (struct FXNodeLines*)ListFindElement1_num(&fxs->fxnodelines->l, pos);
      fxnodeline->logtype = dat.logtype;      
    }

  } else {

    // ONE ELEMENT
    
    const FXText &vt = fxtext.at(0);
    struct FXNodeLines *fxnodeline = vt.fxnodeline;
  
    if (key == EVENT_DEL) {

      if (subsubtrack == 2)
        fxnodeline->logtype = LOGTYPE_LINEAR;
      else
        DeleteFxNodeLine(window, wtrack, fxs, fxnodeline);
      
    } else {

      data_as_text_t dat = DAT_get_overwrite(vt.value, vt.logtype, subsubtrack, key, fx->min, fx->max, true, true);
      printf("fx->min: %x, fx->max: %x, vt.value: %x, dat.value: %x\n",fx->min,fx->max,vt.value,dat.value);

      if (dat.is_valid==false)
        return false;

      fxnodeline->val = dat.value;
      fxnodeline->logtype = dat.logtype;
      
    }    
  }

  return true;
}
  


