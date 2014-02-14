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
#include "playerclass.h"
#include "PEQmempool_proc.h"
#include "OS_Ptask2Mtask_proc.h"
#include "PEQcommon_proc.h"
#include "realline_calc_proc.h"
#include "list_proc.h"
#include "placement_proc.h"

#include "../audio/SoundPlugin.h"
#include "../audio/Pd_plugin_proc.h"

#include "PEQrealline_proc.h"

extern PlayerClass *pc;

extern struct Root *root;

void PlayerNewRealline(struct PEventQueue *peq,int doit);
static void PlayerFirstRealline(struct PEventQueue *peq,int doit);

void InitPEQrealline(struct Blocks *block,Place *place){
	int addplaypos=0;
	int realline;
	struct WBlocks *wblock;
	struct Tracker_Windows *window=root->song->tracker_windows;
	struct PEventQueue *peq=NULL;

	while(window!=NULL){

		wblock=(struct WBlocks *)ListFindElement1(&window->wblocks->l,block->l.num);

		realline=FindRealLineFor(wblock,0,place);

		if(realline>=wblock->num_reallines){
			realline=0;
			wblock=(struct WBlocks *)ListFindElement1(
                                                                  &window->wblocks->l,PC_GetPlayBlock(1)->l.num
                                                                  );
			addplaypos=1;
		}

		if(realline==0) {
                  peq=GetPEQelement();
                  peq->TreatMe=PlayerFirstRealline;
                  PC_InsertElement2_latencycompencated(
                                                       peq, addplaypos,&wblock->reallines[0]->l.p
                                                       );
                  realline = 1;
                }

		peq=GetPEQelement();
		peq->TreatMe=PlayerNewRealline;
		peq->window=window;
		peq->wblock=wblock;
		peq->realline=realline;

		PC_InsertElement2_latencycompencated(
			peq, addplaypos,&wblock->reallines[realline]->l.p
		);
		//printf("time: %d, addplaypos: %d, realline: %d, wblocknum: %d\n",(int)peq->l.time,(int)addplaypos,realline,wblock->l.num);
                //fflush(stdout);

		window=NextWindow(window);
	}
	debug("init. peq->realline: %d\n",peq->realline);
}


static void PlayerFirstRealline(struct PEventQueue *peq,int doit){
	Place firstplace;
	PlaceSetFirstPos(&firstplace);

        RT_PD_set_time(peq->l.time, &firstplace);

        ReturnPEQelement(peq);
}


void PlayerNewRealline(struct PEventQueue *peq,int doit){
	int addplaypos=0;
	int realline=peq->realline;
	//int orgrealline=realline;
	struct Blocks *nextblock;

        RT_PD_set_time(peq->l.time, &peq->wblock->reallines[peq->realline]->l.p);

	peq->wblock->till_curr_realline=realline;

	if(doit){
		Ptask2Mtask();
	}

	realline++;
	if(pc->playtype==PLAYRANGE){
		if(realline>=peq->wblock->rangey2){
			realline=peq->wblock->rangey1;
		}
	}else{
		if(realline>=peq->wblock->num_reallines){
		  nextblock=PC_GetPlayBlock(1);
			if(nextblock==NULL){
				ReturnPEQelement(peq);
				return;
			}else{

                          Place firstplace;
                          PlaceSetFirstPos(&firstplace);

                          struct PEventQueue *peq2=GetPEQelement();
                          peq2->TreatMe=PlayerFirstRealline;
                          PC_InsertElement2_latencycompencated(
                                                               peq2, addplaypos,&firstplace
                                                               );
                          
                          realline=1;
                          peq->wblock= (struct WBlocks *)ListFindElement1(
                                                                          &peq->window->wblocks->l,nextblock->l.num
                                                                          );
                          addplaypos=1;
			}
		}
	}

	peq->realline=realline;

        PC_InsertElement2_latencycompencated(
                                             peq, addplaypos ,&peq->wblock->reallines[realline]->l.p
                                             );


	//printf("NewRealline: %d, time: %d, nextrealline: %d, nexttime: %d, addplaypos: %d, pc->seqtime: %d\n",(int)orgrealline,(int)time,(int)peq->realline,(int)peq->l.time,(int)addplaypos,(int)pc->seqtime);
        //fflush(stdout);

	return;
}


