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
#include "OS_Bs_edit_proc.h"
#include "blocks_proc.h"
#include "wblocks_proc.h"
#include "list_proc.h"
#include "undo_block_insertdelete_proc.h"
#include "player_proc.h"

#include "block_insert_proc.h"


extern struct Root *root;

void InsertBlock_IncBlockNums(
	NInt blockpos
){
	struct Tracker_Windows *window=root->song->tracker_windows;
	struct WBlocks *wblock;
	struct Blocks *block=ListFindElement1_r0(&root->song->blocks->l,blockpos);

	while(block!=NULL){
		block->l.num++;
		block=NextBlock(block);
	}
	root->song->num_blocks++;

	while(window!=NULL){
		wblock=ListFindElement1_r0(&window->wblocks->l,blockpos);
		while(wblock!=NULL){
			wblock->l.num++;
			wblock=NextWBlock(wblock);
		}
		window=NextWindow(window);
	}
}

void InsertBlock(
	NInt blockpos,
	NInt num_tracks,
	int num_lines,
	char *name
){
	struct Tracker_Windows *window=root->song->tracker_windows;
	struct WBlocks *wblock;

	struct Blocks *block;

	InsertBlock_IncBlockNums(blockpos);

	block=talloc(sizeof(struct Blocks));

	block->l.num=blockpos;
	NewBlock(block,num_tracks,num_lines,name);

	while(window!=NULL){
		wblock=talloc(sizeof(struct WBlocks));
		wblock->l.num=blockpos;

		NewWBlock(window,wblock,block);

		window=NextWindow(window);
	}

}


void InsertBlock_CurrPos(
	struct Tracker_Windows *window
){
	struct WBlocks *wblock=window->wblock;
	NInt blockpos=wblock->l.num;

	PlayStop();

	Undo_Block_Insert(blockpos);

	InsertBlock(blockpos,wblock->block->num_tracks,wblock->block->num_lines,"NN");

	SelectWBlock(window,(struct WBlocks *)ListFindElement1(&window->wblocks->l,blockpos));

	BS_UpdateBlockList();
	BS_UpdatePlayList();
}

