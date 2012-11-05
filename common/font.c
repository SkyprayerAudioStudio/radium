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
#include "reallines_proc.h"
#include "windows_proc.h"
#include "wblocks_proc.h"
#include "OS_visual_input.h"


void IncFontSize_CurrPos(
	struct Tracker_Windows *window,
	int incnum
){
	struct WBlocks *wblock=window->wblock;

        if(window->fontheight+incnum<=3)
          incnum = -(window->fontheight-3);

	if(
		incnum==0 ||
		(window->fontheight<5 && incnum<0) ||
		(window->fontheight>126 && incnum>0) ||
		((wblock->t.y2-wblock->t.y1)/(window->fontheight+incnum)<4 && incnum>0)
	) return;

	//window->fontheight+=incnum;
        GFX_IncFontSize(window, incnum);
	//	UpdateReallinesDependens(window,wblock);

        UpdateAllWBlockWidths(window);
	DrawUpTrackerWindow(window);
}

void SetFontSizeNormal_CurrPos(
	struct Tracker_Windows *window
){
  //IncFontSize_CurrPos(window,window->org_fontheight-window->fontheight+2);
  GFX_ResetFontSize(window);
  UpdateAllWBlockWidths(window);
  DrawUpTrackerWindow(window);
}

