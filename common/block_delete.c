

#include "nsmtracker.h"
#include "list_proc.h"
#include "wblocks_proc.h"
#include "OS_Bs_edit_proc.h"
#include "undo_block_insertdelete_proc.h"
#include "player_proc.h"
#include "blocklist_proc.h"

#include "block_delete_proc.h"

extern struct Root *root;

void DeleteBlock(
	NInt blockpos
){
	struct Tracker_Windows *window=root->song->tracker_windows;
	struct WBlocks *wblock;
	struct Blocks *removed_block=ListFindElement1(&root->song->blocks->l,blockpos);
	struct Blocks *nextblock=NextBlock(removed_block);

	ListRemoveElement1(&root->song->blocks,&removed_block->l);

        {
          struct Blocks *block = nextblock;
          while(block!=NULL){
            block->l.num--;
            block=NextBlock(block);
          }
        }

	root->song->num_blocks--;

	while(window!=NULL){
		wblock=ListFindElement1(&window->wblocks->l,blockpos);
		ListRemoveElement1(
			&window->wblocks,
			&wblock->l
		);
		wblock=NextWBlock(wblock);
		while(wblock!=NULL){
			wblock->l.num--;
			wblock=NextWBlock(wblock);
		}
		window=NextWindow(window);
	}

        // Call BL_removeBlockFromPlaylist after blocklist is updated.
        BL_removeBlockFromPlaylist(removed_block);
}


void DeleteBlock_CurrPos(
	struct Tracker_Windows *window
){
	struct WBlocks *wblock=window->wblock;
	NInt blockpos;

	PlayStop();

	if(wblock->l.next==NULL && wblock==window->wblocks) return;

	blockpos=window->wblock->l.num;

	Undo_Block_Delete(blockpos);

	DeleteBlock(blockpos);

	wblock=ListFindElement1_r0(&window->wblocks->l,blockpos);

	if(wblock==NULL){
		wblock=ListLast1(&window->wblocks->l);
	}

	SelectWBlock(window,wblock);

	BS_UpdateBlockList();
	BS_UpdatePlayList();
}


