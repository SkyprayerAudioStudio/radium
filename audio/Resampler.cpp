/* Copyright 2012 Kjetil S. Matheussen

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


#ifndef TEST_MAIN

  #include "../common/nsmtracker.h"
  #include "Resampler_proc.h"

#else

  #include <stdio.h>
  #include <samplerate.h>

  enum{
    RESAMPLER_NON=0,
    RESAMPLER_LINEAR=1,
    RESAMPLER_CUBIC=2,
    RESAMPLER_SINC1=3,
    RESAMPLER_SINC2=4
  };

  struct Root{
    bool editonoff;
  };

  struct Root das_root;
  struct Root *root = &das_root;

  #define ATOMIC_GET(a) a

  #define R_ASSERT_NON_RELEASE(a)

  #define R_MAX(a,b) (((a)>(b))?(a):(b))
  #define R_MIN(a,b) (((a)<(b))?(a):(b))

#endif





namespace{
  

#include "SampleInterpolator.cpp"


struct Resampler{
  virtual ~Resampler(){}
  
  //virtual void init(src_callback_t callback, int num_channels, void *callback_arg, void *other_init_args) = 0;
  virtual int read(double ratio,int num_frames, float *out) = 0;
  virtual void reset() = 0;
};


struct InterpolatingResampler : public Resampler{

  Interpolator m_interpolator;

  src_callback_t m_callback;
  void *m_callback_arg;
  
  float *m_in;
  int    m_num_in_frames_left;
  
  InterpolatingResampler(src_callback_t callback, int num_channels, void *callback_arg)
    : m_callback(callback)
    , m_callback_arg(callback_arg)
    , m_in(NULL)
    , m_num_in_frames_left(0)
  {
  }

private:
  
  int read_internal(
                    double actual_ratio,
                    int num_out_frames_left,
                    float *out,
                    int total_num_frames_produced
                    )
  {
    
    if (num_out_frames_left==0)
      return total_num_frames_produced;

    if (m_num_in_frames_left == 0)
      m_num_in_frames_left = m_callback(m_callback_arg, &m_in);
    
    if (m_num_in_frames_left == 0)
      return total_num_frames_produced;

    int num_frames_consumed;    
    int num_frames_produced = m_interpolator.process(
                                                     actual_ratio,
                                                     m_in,
                                                     m_num_in_frames_left,
                                                     out,
                                                     num_out_frames_left,
                                                     &num_frames_consumed
                                                     );
    
    R_ASSERT_NON_RELEASE(num_frames_consumed <= m_num_in_frames_left);
    R_ASSERT_NON_RELEASE(num_frames_produced <= num_out_frames_left);
    
    m_num_in_frames_left  -= num_frames_consumed;
    m_in                  += num_frames_consumed;

    return read_internal(
                         actual_ratio,
                         num_out_frames_left       - num_frames_produced,
                         out                       + num_frames_produced,
                         total_num_frames_produced + num_frames_produced
                         );
  }
    
public:
  
  int read(double ratio, int num_out_frames_left, float *out) {
    return read_internal(
                         1.0 / ratio,
                         num_out_frames_left,
                         out,
                         0);
  }
  
  void reset() {
    m_interpolator.reset();
    m_num_in_frames_left = 0;
  }

};


struct SincResampler : public Resampler{
  SRC_STATE *_src_state;
  double _last_ratio;

  SincResampler(src_callback_t callback, int num_channels, void *arg, int type){
    //_src_state = src_callback_new(callback, SRC_SINC_FASTEST, num_channels, NULL, arg);
    const static int typemap[]={SRC_ZERO_ORDER_HOLD,SRC_LINEAR,-1,SRC_SINC_FASTEST,SRC_SINC_BEST_QUALITY};
    _src_state = src_callback_new(callback, typemap[type], num_channels, NULL, arg);
    _last_ratio = -1.0;
  }
  ~SincResampler(){
    src_delete(_src_state);
  }

  int read(double ratio,int num_frames, float *out){
    if(_last_ratio != ratio){
      if(_last_ratio >= 0.0)
        src_set_ratio(_src_state, ratio);
      _last_ratio = ratio;
    }

    int ret = src_callback_read(_src_state, ratio, num_frames, out);

    if(ret==0){
      if(src_error(_src_state)!=0)
        printf("Error? %d: \"%s\". ratio: %f\n",
               src_error(_src_state),
               src_strerror(src_error(_src_state)),
               ratio);
      return 0;
    }else
      return ret;
  }

  void reset(){
    src_reset(_src_state);
  }
};


} // end anon. namespace



void *RESAMPLER_create(src_callback_t callback, int num_channels, void *arg, int type){
  Resampler *resampler;

  if(type==RESAMPLER_CUBIC)
    resampler = new InterpolatingResampler(callback, num_channels, arg);
  else
    resampler = new SincResampler(callback, num_channels, arg, type);
  
  return resampler;
}

void RESAMPLER_delete(void *res){
  Resampler *resampler = static_cast<Resampler*>(res);
  delete resampler;
}

int RESAMPLER_read(void *res,double ratio,int num_frames, float *out){
  Resampler *resampler = static_cast<Resampler*>(res);
  return resampler->read(ratio,num_frames,out);
}

void RESAMPLER_reset(void *res){
  Resampler *resampler = static_cast<Resampler*>(res);
  return resampler->reset();
}




#ifdef TEST_MAIN

#include <stdlib.h>
#include <stdio.h>

static long src_callback(void *cb_data, float **out_data){
  static float data[1420];
  *out_data = data;
  return 1420;
}

#  include <xmmintrin.h>

static inline float scale(float x, float x1, float x2, float y1, float y2){
  return y1 + ( ((x-x1)*(y2-y1))
                /
                (x2-x1)
                );
}

int main(int argc, char **argv){
  
  _mm_setcsr(_mm_getcsr() | 0x8040);

  int num_frames = atoi(argv[1]); // If we hardcode the value, the compiler is likely to optimize things which it wouldn't have done normally
  //fprintf(stderr, "num_frames: %d\n",num_frames);
  
  float *data = (float*)malloc(sizeof(float)*num_frames);
 
  for(int i=0;i<num_frames;i++){
    data[i] = (float)i / (float)num_frames; // Insert some some legal values.
  }

  void *resampler = RESAMPLER_create(src_callback, 1, NULL, RESAMPLER_CUBIC);

  int top = 1024*1024*2 * 64 / num_frames;
  
  for(int i=0 ; i < top ; i ++)
    RESAMPLER_read(resampler, scale(0,0,top,0.1,8.0), num_frames, data);
  
  //printf("hello\n");
  return 0;
}
#endif
