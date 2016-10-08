
#include "nsmtracker.h"
#include "placement_proc.h"
#include "patch_proc.h"


#include "scheduler_proc.h"



#define g_num_fx_args 7

static void RT_schedule_fxnodeline(
                                   const struct SeqBlock *seqblock,
                                   const struct Tracks *track,
                                   struct FX *fx,
                                   const struct FXNodeLines *fxnodeline1,
                                   Place start_place
                                   );

static int64_t RT_scheduled_fx(int64_t time, union SuperType *args){
  const struct SeqBlock    *seqblock    = args[0].const_pointer;
  const struct Tracks      *track       = args[1].const_pointer;
  struct FX                *fx          = args[2].pointer;
  const struct FXNodeLines *fxnodeline1 = args[3].const_pointer;
  int64_t                   time1       = args[4].int_num;
  int64_t                   time2       = args[5].int_num;
  int                       last_val    = args[6].int32_num;

  const struct FXNodeLines *fxnodeline2 = NextFXNodeLine(fxnodeline1);
  if (fxnodeline2==NULL) // Can happen if deleting a nodeline while playing.
    return DONT_RESCHEDULE;
  
  struct Patch *patch = track->patch;
  
  // May happen if removing patch from track while playing, not sure. Doesn't hurt to have this check.
  if (fx->patch != patch)
    return DONT_RESCHEDULE;

  int val1 = fxnodeline1->val;
  int val2 = fxnodeline2->val;
  
  int x;
  FX_when when;
  
  if (time==time1) {
    
    when = FX_start;
    x = val1;
    
  } else if (time==time2) {
    
    when = FX_end;
    x = val2;
    
  } else {
    
    when = FX_middle;
    x = scale(time, time1, time2, val1, val2);
    
  }

  if (when!=FX_middle || x != last_val){   // Note: We don't check if last value was similar when sending out FX_end. FX_start and FX_end is always sent to the instrument.

    //printf("   Sending out %d at %d\n",x,(int)time);
    
    RT_FX_treat_fx(fx, x, time, 0, when);

    float *slider_automation_value = ATOMIC_GET(fx->slider_automation_value);
    if(slider_automation_value!=NULL)
      safe_float_write(slider_automation_value, scale(x,fx->min,fx->max,0.0f,1.0f));
    
    enum ColorNums *slider_automation_color = ATOMIC_GET(fx->slider_automation_color);    
    if(slider_automation_color!=NULL)
      __atomic_store_n(slider_automation_color, fx->color, __ATOMIC_SEQ_CST);
  }
  
  if (time==time2) { // If we check "when==FX_end" instead, we go into an infinte loop if time==time1==time2.
    
    RT_schedule_fxnodeline(seqblock, track, fx, fxnodeline2, fxnodeline2->l.p);
    return DONT_RESCHEDULE;
    
  } else {

    args[6].int32_num = x;

    if (fxnodeline1->logtype == LOGTYPE_HOLD)
      return time2;
    else
      return R_MIN(time + RADIUM_BLOCKSIZE, time2);
  }
    
}



static void RT_schedule_fxnodeline(
                                   const struct SeqBlock *seqblock,
                                   const struct Tracks *track,
                                   struct FX *fx,
                                   const struct FXNodeLines *fxnodeline1,
                                   Place start_place
                                   )
{
  R_ASSERT_RETURN_IF_FALSE(fxnodeline1!=NULL);

  const struct FXNodeLines *fxnodeline2 = NextFXNodeLine(fxnodeline1); // Might be NULL.
  if (fxnodeline2==NULL)
    return;
  
  int64_t time1 = get_seqblock_place_time(seqblock, fxnodeline1->l.p);
  int64_t time2 = get_seqblock_place_time(seqblock, fxnodeline2->l.p);

  int64_t time;
  if (PlaceEqual(&start_place, &fxnodeline1->l.p)) // This test is not really necessary, get_seqblock_place_time should always return the same value for the same place. But with this check there is no need to think about the possibility of it to fail.
    time = time1;
  else
    time = get_seqblock_place_time(seqblock, start_place);
  
  union SuperType args[g_num_fx_args];
  args[0].const_pointer = seqblock;
  args[1].const_pointer = track;
  args[2].pointer       = fx;
  args[3].const_pointer = fxnodeline1;
  args[4].int_num       = time1;
  args[5].int_num       = time2;
  args[6].int32_num     = INT32_MIN;
  
  //printf(" Scheduling FX at %d. seqblock->time: %d\n",(int)time, (int)seqblock->time);
  SCHEDULER_add_event(time, RT_scheduled_fx, &args[0], g_num_fx_args, SCHEDULER_FX_PRIORITY);
}


void RT_schedule_fxs_newblock(const struct SeqTrack *seqtrack,
                              const struct SeqBlock *seqblock,
                              int64_t start_time,
                              Place start_place)
{
  struct Tracks *track=seqblock->block->tracks;
  
  while(track!=NULL){

    VECTOR_FOR_EACH(struct FXs *fxs, &track->fxs){

      struct FXNodeLines *fxnodeline1 = fxs->fxnodelines;

      if (PlaceGreaterOrEqual(&fxnodeline1->l.p, &start_place)){
        
        RT_schedule_fxnodeline(seqblock, track, fxs->fx, fxnodeline1, fxnodeline1->l.p);
        
      } else {

        struct FXNodeLines *fxnodeline2 = NextFXNodeLine(fxnodeline1);

        while(fxnodeline2 != NULL){
          if (PlaceGreaterOrEqual(&start_place, &fxnodeline1->l.p) && PlaceLessThan(&start_place, &fxnodeline2->l.p)) {
            RT_schedule_fxnodeline(seqblock, track, fxs->fx, fxnodeline1, start_place);
            break;
          }
          fxnodeline1 = fxnodeline2;
          fxnodeline2 = NextFXNodeLine(fxnodeline2);
        }
        
      }
      
    }END_VECTOR_FOR_EACH;

    
    track=NextTrack(track);   
  }

}


