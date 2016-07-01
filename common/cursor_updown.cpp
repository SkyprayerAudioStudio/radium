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




#include "nsmtracker.h"
#include "vector_proc.h"
#include "gfx_wblocks_proc.h"
#include "windows_proc.h"
#include "sliders_proc.h"
#include "visual_proc.h"
#include "common_proc.h"
#include "wtracks_proc.h"
#include "wblocks_proc.h"
#include "pixmap_proc.h"
#include "scroll_proc.h"
#include "scroll_play_proc.h"
#include "blts_proc.h"
#include "list_proc.h"
#include "trackreallines2_proc.h"
#include "centtext_proc.h"
#include "chancetext_proc.h"
#include "veltext_proc.h"
#include "fxtext_proc.h"
#include "realline_calc_proc.h"
#include "../OpenGL/Widget_proc.h"

#include "../api/api_proc.h"

#include "cursor_updown_proc.h"

extern int g_downscroll;

int getScrollMultiplication(void){
  if (doScrollEditLines())
    return R_MAX(1, g_downscroll);
  else
    return 1;
}

void ScrollEditorDown(struct Tracker_Windows *window,int num_lines){
	struct WBlocks *wblock;

	if(num_lines==0) return;

	wblock=window->wblock;

	if(
		wblock->curr_realline<wblock->num_reallines-1 &&
		wblock->curr_realline+num_lines>wblock->num_reallines-1
	){
		num_lines=wblock->num_reallines - wblock->curr_realline;
	}

	if(num_lines/getScrollMultiplication()==1 || num_lines/getScrollMultiplication()==-1)
		Scroll_play_down(wblock,wblock->curr_realline,wblock->curr_realline+num_lines-1);

	if(wblock->curr_realline+num_lines < wblock->num_reallines){
	  Scroll_scroll(window,num_lines);
	}else{
		/* When on the bottom line. */

//		RError("top: %d, num/2: %d\n",wblock->top_realline,wblock->num_visiblelines/2);
		if(wblock->top_realline <= wblock->num_visiblelines/2){
//			RError("jepp\n");
			Scroll_scroll(window,-wblock->curr_realline);

		}else{

			wblock->curr_realline=0;
                        GE_set_curr_realline(0);

			SetWBlock_Top_And_Bot_Realline(window,wblock);

#if !USE_OPENGL
			PixMap_reset(window);

                        struct WTracks *wtrack=ListLast1(&wblock->wtracks->l);
                        int x2=wtrack->fxarea.x2;

                        /*
			GFX_FilledBox(
				window,
				0,
				wblock->a.x1,wblock->t.y1,
				x2,wblock->t.y2,
                                PAINT_BUFFER
			);
                        */
                        EraseAllLines(window, wblock, wblock->a.x1, x2);

			DrawWBlockSpesific(
				window,
				wblock,
				0,
				wblock->num_visiblelines
			);

			UpdateAllWTracks(
				window,
				wblock,
				0,
				wblock->num_visiblelines
			);
			/*
			GFX_FilledBox(
				      window,
				      0,
				      wblock->a.x1,wblock->t.y1,
				      wblock->t.x2,
				      Common_oldGetReallineY2Pos(window,wblock,wblock->curr_realline-1)
				      );
			*/
#endif
		}
	}

#if !USE_OPENGL
	Blt_scrollMark(window);

	UpdateLeftSlider(window);
#endif
}


void MaybeScrollEditorDownAfterEditing(struct Tracker_Windows *window){
  if(!is_playing() || ATOMIC_GET(root->play_cursor_onoff)==true)
    ScrollEditorDown(window,g_downscroll);
}

void ScrollEditorUp(struct Tracker_Windows *window,int num_lines){
	struct WBlocks *wblock;

	if(num_lines==0) return;

	wblock=window->wblock;

	if(wblock->curr_realline>0 && wblock->curr_realline-num_lines<0){
		num_lines=wblock->curr_realline;
	}

	if(num_lines/getScrollMultiplication()==1 || num_lines/getScrollMultiplication()==-1)
          Scroll_play_up(wblock,wblock->curr_realline-num_lines+1,wblock->curr_realline);

	if(wblock->curr_realline-num_lines>=0){

	  Scroll_scroll(window,-num_lines);
	}else{

		if(wblock->bot_realline >= (wblock->num_reallines-(wblock->num_visiblelines/2)-1)){
			Scroll_scroll(window,wblock->num_reallines-1);
		}else{
			wblock->curr_realline=wblock->num_reallines-1;
                        GE_set_curr_realline(wblock->curr_realline);

			SetWBlock_Top_And_Bot_Realline(window,wblock);

#if !USE_OPENGL
			PixMap_reset(window);

                        struct WTracks *wtrack=ListLast1(&wblock->wtracks->l);
                        int x2=wtrack->fxarea.x2;

                        EraseAllLines(window, wblock, wblock->a.x1, x2);

			DrawWBlockSpesific(
				window,
				wblock,
				wblock->top_realline,
				wblock->curr_realline
			);

			UpdateAllWTracks(
				window,
				wblock,
				wblock->top_realline,
				wblock->curr_realline
			);

			/*
			GFX_FilledBox(
				      window,
				      0,
				      wblock->a.x1,Common_oldGetReallineY1Pos(window,wblock,wblock->curr_realline+1),
				      wblock->t.x2,
				      wblock->t.y2
			      );
			*/
#endif
		}
	}

#if !USE_OPENGL
	Blt_scrollMark(window);

	UpdateLeftSlider(window);
#endif
}

template <class T>
static void scroll_next(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, const QMap<int, T> &trss){
	int curr_realline=wblock->curr_realline;

        if(curr_realline==wblock->num_reallines-1){ // last line
          ScrollEditorDown(window,1);
          return;
        }

	int new_realline;

        for(new_realline=curr_realline+1 ; new_realline < wblock->num_reallines-1 ; new_realline++)
          if (trss.contains(new_realline))
            break;

	ScrollEditorDown(window,new_realline-curr_realline);
}

template <class T>
static void scroll_prev(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, const QMap<int, T> &trss){
	int curr_realline=wblock->curr_realline;

        if (curr_realline==0){
          ScrollEditorUp(window,1);
          return;
	}

        int new_realline;

        for(new_realline=curr_realline-1 ; new_realline > 0 ; new_realline--)
          if (trss.contains(new_realline))
            break;

	ScrollEditorUp(window,curr_realline-new_realline);
}

void ScrollEditorNextNote(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack){
        const Trss &trss = TRSS_get(wblock, wtrack);

        scroll_next(window, wblock, wtrack, trss);
}

void ScrollEditorPrevNote(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack){
        const Trss &trss = TRSS_get(wblock, wtrack);
        scroll_prev(window, wblock, wtrack, trss);
}

static Waveform_trss get_waveform_trss(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, int polyphony_num){
  Waveform_trss trss;
  
  struct Notes *note = wtrack->track->notes;
  
  while(note != NULL){
    if (note->polyphony_num == polyphony_num) {
      int realline = FindRealLineForNote(wblock, 0, note);
      trss[realline] = true;
      int realline2 = FindRealLineForEndNote(wblock, 0, note);
      trss[realline2] = true;
    }
    note = NextNote(note);
  }

  return trss;
}


void ScrollEditorNextWaveform(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, int polyphony_num){
  const Waveform_trss &trss = get_waveform_trss(window, wblock, wtrack, polyphony_num);
  scroll_next(window, wblock, wtrack, trss);
}

void ScrollEditorPrevWaveform(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, int polyphony_num){
  const Waveform_trss &trss = get_waveform_trss(window, wblock, wtrack, polyphony_num);
  scroll_prev(window, wblock, wtrack, trss);
}

void ScrollEditorNextVelocity(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack){
  const VelText_trss &trss = VELTEXTS_get(wblock, wtrack);
  scroll_next(window, wblock, wtrack, trss);
}

void ScrollEditorPrevVelocity(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack){
  const VelText_trss &trss = VELTEXTS_get(wblock, wtrack);
  scroll_prev(window, wblock, wtrack, trss);
}

void ScrollEditorNextFx(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, struct FXs *fxs){
  const FXText_trss &trss = FXTEXTS_get(wblock, wtrack, fxs);
  scroll_next(window, wblock, wtrack, trss);
}

void ScrollEditorPrevFx(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack, struct FXs *fxs){
  const FXText_trss &trss = FXTEXTS_get(wblock, wtrack, fxs);
  scroll_prev(window, wblock, wtrack, trss);
}

void ScrollEditorNextSomething(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack){
  if (window->curr_track < 0 || window->curr_track_sub==-1 || CENTTEXT_subsubtrack(window, wtrack)!=-1 || CHANCETEXT_subsubtrack(window, wtrack)!=-1){
    ScrollEditorNextNote(window, wblock, wtrack);
    return;
  }

  int curr_polyphony_num = window->curr_track_sub - WTRACK_num_non_polyphonic_subtracks(wtrack);

  if (curr_polyphony_num >= 0){
    ScrollEditorNextWaveform(window, wblock, wtrack, curr_polyphony_num);
    return;
  }

  if (VELTEXT_subsubtrack(window, wtrack) != -1){
    ScrollEditorNextVelocity(window, wblock, wtrack);
    return;
  }

  struct FXs *fxs;  
  if (FXTEXT_subsubtrack(window, wtrack, &fxs) != -1){
    ScrollEditorNextFx(window, wblock, wtrack, fxs);
    return;
  }

}

void ScrollEditorPrevSomething(struct Tracker_Windows *window, struct WBlocks *wblock, struct WTracks *wtrack){
  if (window->curr_track < 0 || window->curr_track_sub==-1 || CENTTEXT_subsubtrack(window, wtrack)!=-1 || CHANCETEXT_subsubtrack(window, wtrack)!=-1){
    ScrollEditorPrevNote(window, wblock, wtrack);
    return;
  }

  int curr_polyphony_num = window->curr_track_sub - WTRACK_num_non_polyphonic_subtracks(wtrack);

  if (curr_polyphony_num >= 0){
    ScrollEditorPrevWaveform(window, wblock, wtrack, curr_polyphony_num);
    return;
  }
  
  if (VELTEXT_subsubtrack(window, wtrack) != -1){
    ScrollEditorPrevVelocity(window, wblock, wtrack);
    return;
  }

  struct FXs *fxs;  
  if (FXTEXT_subsubtrack(window, wtrack, &fxs) != -1){
    ScrollEditorPrevFx(window, wblock, wtrack, fxs);
    return;
  }
}

void ScrollEditorToRealLine(
	struct Tracker_Windows *window,
	struct WBlocks *wblock,
	int till_curr_realline
){
	int curr_realline=wblock->curr_realline;

	//printf("Going to scroll to line %d. Now: %d \n",till_curr_realline,curr_realline);
/*
	if(till_curr_realline<0){
		till_curr_realline=wblock->num_reallines-1;
	}

	if(till_curr_realline>=wblock->num_reallines){
		till_curr_realline=0;
	}
*/

	if( till_curr_realline < curr_realline ){
		ScrollEditorUp(
			window,
			curr_realline - till_curr_realline
		);
	}else{
		if( till_curr_realline > curr_realline ){
			ScrollEditorDown(
				window,
				till_curr_realline - curr_realline
			);
		}
	}
}

void ScrollEditorToRealLine_CurrPos(
	struct Tracker_Windows *window,
	int till_curr_realline
){
	ScrollEditorToRealLine(window,window->wblock,till_curr_realline);
}

/*
void ScrollEditor(
	struct Tracker_Windows *window,
	int num_reallines
){
	struct WBlocks *wblock=window->wblock;
	int curr_realline=wblock->curr_realline;

	ScrollEditorToRealLine(window,wblock,curr_realline+num_reallines);
}
*/

void ScrollEditorToLine_CurrPos(
	struct Tracker_Windows *window,
	int line
){
	struct WBlocks *wblock=window->wblock;

        int realline = FindRealLineFor(wblock, 0, PlaceCreate2(line));
	ScrollEditorToRealLine(window,wblock,realline);
}

void ScrollEditorToPercentLine_CurrPos(
	struct Tracker_Windows *window,
	int percent
){
	struct WBlocks *wblock=window->wblock;
	struct Blocks *block=wblock->block;

	int line=block->num_lines*percent/100;

	ScrollEditorToLine_CurrPos(window,line);
}
