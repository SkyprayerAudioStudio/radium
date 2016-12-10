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

#ifdef __cplusplus
extern "C" {
#endif

extern void callMeBeforeReturningToS7(void);
extern void handleError(const char *fmt,...);

extern struct Tracker_Windows *getWindowFromNum(int windownum);


extern struct WBlocks *getWBlockFromNum(int windownum,int wblocknum);


extern struct WBlocks *getWBlockFromNumA(
	int windownum,
	struct Tracker_Windows **window,
	int blocknum
);

extern struct Blocks *getBlockFromNum(int blocknum);

extern struct Tracks *getTrackFromNum(int blocknum,int tracknum);

extern struct WTracks *getWTrackFromNum(
	int windownum,
	int blocknum,
	int wtracknum
);


extern struct WTracks *getWTrackFromNumA(
	int windownum,
	struct Tracker_Windows **window,
	int wblocknum,
	struct WBlocks **wblock,
	int wtracknum
);

extern struct Notes *getNoteFromNum(int windownum,int blocknum,int tracknum,int notenum);
extern struct Notes *getNoteFromNumA(int windownum,struct Tracker_Windows **window, int blocknum, struct WBlocks **wblock, int tracknum, struct WTracks **wtrack, int notenum);

extern struct Pitches *getPitchFromNumA(int windownum,struct Tracker_Windows **window, int blocknum, struct WBlocks **wblock, int tracknum, struct WTracks **wtrack, int notenum, struct Notes **note, int pitchnum);
extern struct Pitches *getPitchFromNum(int windownum,int blocknum,int tracknum,int notenum,int pitchnum);

extern struct Velocities *getVelocityFromNumA(int windownum,struct Tracker_Windows **window, int blocknum, struct WBlocks **wblock, int tracknum, struct WTracks **wtrack, int notenum, struct Notes **note, int velocitynum);
extern struct Velocities *getVelocityFromNum(int windownum,int blocknum,int tracknum,int notenum,int velocitynum);

extern struct Signatures *getSignatureFromNumA(int windownum,struct Tracker_Windows **window, int blocknum, struct WBlocks **wblock, int num);
extern struct Signatures *getSignatureFromNum(int windownum,int blocknum,int num);

extern struct LPBs *getLPBFromNumA(int windownum,struct Tracker_Windows **window, int blocknum, struct WBlocks **wblock, int num);
extern struct LPBs *getLPBFromNum(int windownum,int blocknum,int num);

extern struct BPMs *getBPMFromNumA(int windownum,struct Tracker_Windows **window, int blocknum, struct WBlocks **wblock, int bpmnum);
extern struct BPMs *getBPMFromNum(int windownum,int blocknum,int bpmnum);

extern struct FXs *getFXsFromNumA(int windownum,struct Tracker_Windows **window, int blocknum, struct WBlocks **wblock, int tracknum, struct WTracks **wtrack, int fxnum);
extern struct FXs *getFXsFromNum(int windownum,int blocknum,int tracknum,int fxnum);

extern struct Patch *getPatchFromNum(int64_t instrument_id);
extern struct Patch *getAudioPatchFromNum(int64_t instrument_id);

extern struct SeqTrack *getSeqtrackFromNum(int seqtracknum);
extern struct SeqBlock *getSeqblockFromNum(int seqblocknum, int seqtracknum);
extern struct SeqBlock *getSeqblockFromNumA(int seqblocknum, int seqtracknum, struct SeqTrack **seqtrack);
extern struct SeqBlock *getGfxGfxSeqblockFromNumA(int seqblocknum, int seqtracknum, struct SeqTrack **seqtrack);

#ifdef __cplusplus
}
#endif
