

#ifndef _RADIUM_COMMON_SEQTRACK_PROC_H
#define _RADIUM_COMMON_SEQTRACK_PROC_H

#define SEQNAV_SIZE_HANDLE_WIDTH 50
#define SEQUENCER_EXTRA_SONG_LENGTH 30.0 // sequencer gui always shows 30 seconds more than the song length

enum GridType{
  NO_GRID = 0,
  BAR_GRID = 1,
  BEAT_GRID = 1
};


extern LANGSPEC int64_t get_abstime_from_seqtime(const struct SeqTrack *seqtrack, const struct SeqBlock *seqblock, int64_t block_seqtime); // Result is rounded down to nearest integer.
extern LANGSPEC int64_t get_seqtime_from_abstime(const struct SeqTrack *seqtrack, const struct SeqBlock *seqblock_to_ignore, int64_t abstime); // Result is rounded down to nearest integer.
  
// sequencer gfx
extern LANGSPEC float SEQUENCER_get_x1(void);
extern LANGSPEC float SEQUENCER_get_x2(void);
extern LANGSPEC float SEQUENCER_get_y1(void);
extern LANGSPEC float SEQUENCER_get_y2(void);

extern LANGSPEC void SEQUENCER_disable_gfx_updates(void);
extern LANGSPEC void SEQUENCER_enable_gfx_updates(void);

#ifdef __cplusplus
struct SEQUENCER_ScopedGfxDisable{
  SEQUENCER_ScopedGfxDisable(){
    SEQUENCER_disable_gfx_updates();
  }
  ~SEQUENCER_ScopedGfxDisable(){
    SEQUENCER_enable_gfx_updates();
  }
};
#endif

extern LANGSPEC int64_t SEQUENCER_get_visible_start_time(void);
extern LANGSPEC int64_t SEQUENCER_get_visible_end_time(void);
extern LANGSPEC void SEQUENCER_set_visible_start_and_end_time(int64_t start_time, int64_t end_time);
extern LANGSPEC void SEQUENCER_set_visible_start_time(int64_t val);
extern LANGSPEC void SEQUENCER_set_visible_end_time(int64_t val);

extern LANGSPEC void SEQUENCER_set_grid_type(enum GridType grid_type);

extern LANGSPEC void SEQUENCER_hide_because_instrument_widget_is_large(void);
extern LANGSPEC void SEQUENCER_show_because_instrument_widget_is_large(void);


// sequencer navigator gfx
extern LANGSPEC float SEQNAV_get_x1(void);
extern LANGSPEC float SEQNAV_get_x2(void);
extern LANGSPEC float SEQNAV_get_y1(void);
extern LANGSPEC float SEQNAV_get_y2(void);
extern LANGSPEC float SEQNAV_get_left_handle_x(void);
extern LANGSPEC float SEQNAV_get_right_handle_x(void);
extern LANGSPEC void SEQNAV_update(void);
extern LANGSPEC void RT_SEQUENCER_update_sequencer_and_playlist(void);

// seqtrack gfx
extern LANGSPEC float SEQTRACK_get_x1(int seqtracknum);
extern LANGSPEC float SEQTRACK_get_x2(int seqtracknum);
extern LANGSPEC float SEQTRACK_get_y1(int seqtracknum);
extern LANGSPEC float SEQTRACK_get_y2(int seqtracknum);

extern LANGSPEC void SEQTRACK_update(struct SeqTrack *seqtrack);
//extern LANGSPEC void SEQTRACK_recreate(int seqtracknum);


extern LANGSPEC float SEQTEMPO_get_x1(void);
extern LANGSPEC float SEQTEMPO_get_x2(void);
extern LANGSPEC float SEQTEMPO_get_y1(void);
extern LANGSPEC float SEQTEMPO_get_y2(void);
extern LANGSPEC void SEQTEMPO_set_visible(bool visible);
extern LANGSPEC bool SEQTEMPO_is_visible(void);

// seqblocks gfx
extern LANGSPEC float SEQBLOCK_get_x1(int seqblocknum, int seqtracknum);
extern LANGSPEC float SEQBLOCK_get_x2(int seqblocknum, int seqtracknum);
extern LANGSPEC float SEQBLOCK_get_y1(int seqblocknum, int seqtracknum);
extern LANGSPEC float SEQBLOCK_get_y2(int seqblocknum, int seqtracknum);

// sequencer_gfx
extern LANGSPEC void SEQTRACK_update_all_seqblock_start_and_end_times(struct SeqTrack *seqtrack);
extern LANGSPEC void SEQTRACK_update_all_seqblock_gfx_start_and_end_times(struct SeqTrack *seqtrack);
//extern LANGSPEC void SEQUENCER_update_all_seqblock_positions(void);
extern LANGSPEC void SEQUENCER_update(void);

  
// seqtrack
extern LANGSPEC void RT_legalize_seqtrack_timing(struct SeqTrack *seqtrack);

extern LANGSPEC void SEQTRACK_move_all_seqblocks_to_the_right_of(struct SeqTrack *seqtrack, int seqblocknum, int64_t how_much);
extern LANGSPEC void SEQTRACK_delete_seqblock(struct SeqTrack *seqtrack, const struct SeqBlock *seqblock);
extern LANGSPEC void SEQTRACK_move_seqblock(struct SeqTrack *seqtrack, struct SeqBlock *seqblock, int64_t new_time);
extern LANGSPEC void SEQTRACK_move_gfx_seqblock(struct SeqTrack *seqtrack, struct SeqBlock *seqblock, int64_t new_time);
extern LANGSPEC void SEQTRACK_insert_silence(struct SeqTrack *seqtrack, int64_t seqtime, int64_t length);
extern LANGSPEC int SEQTRACK_insert_seqblock(struct SeqTrack *seqtrack, struct SeqBlock *seqblock, int64_t seqtime);
extern LANGSPEC int SEQTRACK_insert_block(struct SeqTrack *seqtrack, struct Blocks *block, int64_t seqtime);
extern LANGSPEC double SEQTRACK_get_length(struct SeqTrack *seqtrack);
extern LANGSPEC double SEQTRACK_get_gfx_length(struct SeqTrack *seqtrack);
extern LANGSPEC void SEQTRACK_init(struct SeqTrack *seqtrack);
extern LANGSPEC struct SeqTrack *SEQTRACK_create(void);
extern LANGSPEC struct SeqTrack *SEQTRACK_create_from_playlist(const int *playlist, int len);


// sequencer
extern LANGSPEC void SEQUENCER_remove_block_from_seqtracks(struct Blocks *block);
extern LANGSPEC int64_t SEQUENCER_find_closest_bar_start(int seqtracknum, int64_t pos_seqtime);
extern LANGSPEC hash_t *SEQUENCER_get_state(void);
extern LANGSPEC void SEQUENCER_create_from_state(hash_t *state);
extern LANGSPEC void SEQUENCER_update_all_seqblock_start_and_end_times(void);
extern LANGSPEC void SEQUENCER_insert_seqtrack(struct SeqTrack *new_seqtrack, int pos);
extern LANGSPEC void SEQUENCER_append_seqtrack(struct SeqTrack *new_seqtrack);
extern LANGSPEC void SEQUENCER_replace_seqtrack(struct SeqTrack *new_seqtrack, int pos);
extern LANGSPEC void SEQUENCER_delete_seqtrack(int pos);

// song
extern LANGSPEC double SONG_get_length(void);
extern LANGSPEC double SONG_get_gfx_length(void);
extern LANGSPEC void SONG_init(void);

#endif

