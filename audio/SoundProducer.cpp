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



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "pa_memorybarrier.h"

#include "../common/nsmtracker.h"
#include "../common/OS_Player_proc.h"

#include "SoundPlugin.h"
#include "SoundPlugin_proc.h"
#include "system_compressor_wrapper_proc.h"

#include "SoundProducer_proc.h"
#include "Mixer_proc.h"

#if 0
faust conversions:
db2linear(x)	= pow(10, x/20.0);
linear2db(x)	= 20*log10(x);
zita output default: curr: -40, min: -70, max: 40
#endif

#if 0

static float scale(float x, float x1, float x2, float y1, float y2){
  return y1 + ( ((x-x1)*(y2-y1))
                /
                (x2-x1)
                );
}

static float linear2db(float val){
  if(val<=0.0f)
    return 0.0f;

  float db = 20*log10(val);
  if(db<-70)
    return 0.0f;
  else if(db>40)
    return 1.0f;
  else
    return scale(db,-70,40,0,1);
}
#endif

struct SoundProducer;

#if 0
// Function iec_scale picked from meterbridge by Steve Harris.
// db is a value between 0 and 1.
static float iec_scale(float db) {
  
  db = 20.0f * log10f(db);


         float def = 0.0f; /* Meter deflection %age */
 
         if (db < -70.0f) {
                 def = 0.0f;
         } else if (db < -60.0f) {
                 def = (db + 70.0f) * 0.25f;
         } else if (db < -50.0f) {
                 def = (db + 60.0f) * 0.5f + 5.0f;
         } else if (db < -40.0f) {
                 def = (db + 50.0f) * 0.75f + 7.5;
         } else if (db < -30.0f) {
                 def = (db + 40.0f) * 1.5f + 15.0f;
         } else if (db < -20.0f) {
                 def = (db + 30.0f) * 2.0f + 30.0f;
         } else if (db < 0.0f) {
                 def = (db + 20.0f) * 2.5f + 50.0f;
         } else {
                 def = 100.0f;
         }
 
         return def * 2.0f / 200.0f;
}
#endif

struct SoundProducerLink : public DoublyLinkedList{
  SoundProducer *sound_producer;
  int sound_producer_ch;
  enum {NO_STATE, FADING_IN, STABLE, FADING_OUT} state;
};

#if 0

static int dllen(DoublyLinkedList *l){
  int ret=0;
  while(l!=NULL){
    ret++;
    l=l->next;
  }
  return ret;
}

static float scale(float x, float x1, float x2, float y1, float y2){
  return y1 + ( ((x-x1)*(y2-y1))
                /
                (x2-x1)
                );
}

#endif

// Example using SSE instructions here: http://codereview.stackexchange.com/questions/5143/min-max-function-of-1d-array-in-c-c
// That code is 4 times faster, but requires SSE.
static float RT_get_max_val(float *array, int num_elements){
  float ret=0.0f;
  float minus_ret = 0.0f;
  
  for(int i=0;i<num_elements;i++){
    float val = array[i];
    if(val>ret){
      ret=val;
      minus_ret = -val;
    }else if (val<minus_ret){
      ret = -val;
        minus_ret = val;
    }
  }
  
  return ret;
}

#if 0
static void RT_apply_volume(float *sound, int num_frames, float volume){
  if(volume!=1.0f)
    for(int i=0;i<num_frames;i++)
      sound[i] *= volume;
}
#endif

#if 0
static void RT_apply_volume2(float *sound, int num_frames, float start_volume, float end_volume){
  if(start_volume==end_volume || fabsf(start_volume-end_volume) < 0.001){
    if(end_volume!=1.0)
      for(int i=0;i<num_frames;i++)
        sound[i] *= end_volume;
  }else{
    float inc = (end_volume-start_volume) / num_frames;
    for(int i=0;i<num_frames;i++){
      start_volume += inc;
      sound[i] *= start_volume;
    }
  }
}
#endif

#if 0
static void RT_copy_sound_and_apply_volume(float *to_sound, float *from_sound, int num_frames, float start_volume, float end_volume){
  if(start_volume==end_volume || fabsf(start_volume-end_volume) < 0.001){
    if(end_volume!=1.0)
      for(int i=0;i<num_frames;i++)
        to_sound[i] = from_sound[i] * end_volume;
    else
      memcpy(to_sound,from_sound,sizeof(float)*num_frames);
  }else{
    float inc = (end_volume-start_volume) / (float)num_frames;
    for(int i=0;i<num_frames;i++){
      start_volume += inc;
      to_sound[i] = from_sound[i]*start_volume;
    }
  }
}
#endif

static void RT_apply_dry_wet(float **dry, int num_dry_channels, float **wet, int num_wet_channels, int num_frames, Smooth *wet_values){
  int num_channels = std::min(num_dry_channels,num_wet_channels);
  for(int ch=0;ch<num_channels;ch++){
    float *w=wet[ch];
    float *d=dry[ch];

    SMOOTH_apply_volume(wet_values, w, num_frames);
    SMOOTH_mix_sounds_using_inverted_values(wet_values, w, d, num_frames);
  }
}

#if 1
static void RT_fade_in(float *sound, int num_frames){
  float num_frames_plus_1 = num_frames+1.0f;
  for(int i=0;i<num_frames;i++)
    sound[i] *= (i+1.0f)/num_frames_plus_1;
}

#else

// not correct
static void RT_fade_in(float *sound, int num_frames){
  float num_frames_plus_1 = num_frames+1.0f;

  float val = 1.0f/num_frames_plus_1;
  float inc = val - ( (num_frames-1) / num_frames_plus_1);

  for(int i=0;i<num_frames;i++){
    sound[i] *= val;
    val += inc;
  }
}
#endif


static void RT_fade_out(float *sound, int num_frames){
  float num_frames_plus_1 = num_frames+1.0f;
  int i;
  float val = (num_frames / num_frames_plus_1);
  float inc = val - ( (num_frames-1) / num_frames_plus_1);

  for(i=0;i<num_frames;i++){
    sound[i] *= val;
    val -= inc;
  }
}

static RSemaphore *signal_from_RT = NULL;

static void PLUGIN_RT_process(SoundPlugin *plugin, int64_t time, int num_frames, float **inputs, float **outputs){
  plugin->type->RT_process(plugin, time, num_frames, inputs, outputs);

#if 0
  static bool printed = false;

  for(int ch=0;ch<plugin->type->num_outputs;ch++)
    for(int i=0;i<num_frames;i++)
      if(!isfinite(outputs[ch][i])){
        if(printed==false)
          printf("NOT FINITE %s %s\n",plugin->type->type_name, plugin->type->name);
        printed=true;
        outputs[ch][i]=0.0f;
      }
#endif
}

struct SoundProducer : DoublyLinkedList{
  SoundPlugin *_plugin;

  int _num_inputs;
  int _num_outputs;
  int _num_dry_sounds;

  int64_t _last_time;

  float **_dry_sound;
  float **_input_sound; // I'm not sure if it's best to place this data on the heap or the stack. Currently the heap is used. Advantage of heap: Avoid (having to think about the possibility of) risking running out of stack. Advantage of stack: Fewer cache misses.
  float **_output_sound;

  float *_input_peaks;
  float *_volume_peaks;

  SoundProducerLink *_input_producers; // one SoundProducerLink per in-channel

  SoundProducer(SoundPlugin *plugin, int num_frames)
    : _plugin(plugin)
    , _num_inputs(plugin->type->num_inputs)
    , _num_outputs(plugin->type->num_outputs)
    , _last_time(-1)
  {
    printf("New SoundProducer. Inputs: %d, Ouptuts: %d\n",_num_inputs,_num_outputs);

    if(_num_inputs>0)
      _num_dry_sounds = _num_inputs;
    else
      _num_dry_sounds = _num_outputs;

    _input_producers = (SoundProducerLink*)calloc(sizeof(SoundProducerLink),_num_inputs);

    allocate_sound_buffers(num_frames);

    _input_peaks = (float*)calloc(sizeof(float),_num_dry_sounds);
    _volume_peaks = (float*)calloc(sizeof(float),_num_outputs);

    MIXER_add_SoundProducer(this);
  }

  ~SoundProducer(){
    MIXER_remove_SoundProducer(this);

    free(_input_peaks);
    free(_volume_peaks);

    free(_input_producers);

    free_sound_buffers();
  }

  void free_sound_buffers(){
    for(int ch=0;ch<_num_dry_sounds;ch++)
      free(_dry_sound[ch]);

    for(int ch=0;ch<_num_inputs;ch++)
      free(_input_sound[ch]);

    for(int ch=0;ch<_num_outputs;ch++)
      free(_output_sound[ch]);

    free(_dry_sound);
    free(_input_sound);
    free(_output_sound);
  }
  
  void RT_set_bus_descendant_type_for_plugin(){
    if(_plugin->bus_descendant_type != MAYBE_A_BUS_DESCENDANT)
      return;

    if(!strcmp(_plugin->type->type_name,"Bus")){
      _plugin->bus_descendant_type = IS_BUS_DESCENDANT;
      return;
    }

    for(int ch=0;ch<_num_inputs;ch++){
      SoundProducerLink *link = (SoundProducerLink*)_input_producers[ch].next;
      while(link!=NULL){
        link->sound_producer->RT_set_bus_descendant_type_for_plugin();
        if(link->sound_producer->_plugin->bus_descendant_type==IS_BUS_DESCENDANT){
          _plugin->bus_descendant_type = IS_BUS_DESCENDANT;
          return;
        }
        
        link=(SoundProducerLink*)link->next;
      }
    }

    _plugin->bus_descendant_type = IS_NOT_A_BUS_DESCENDANT;
  }

  void allocate_sound_buffers(int num_frames){
    _dry_sound = (float**)(calloc(sizeof(float*),_num_dry_sounds));
    for(int ch=0;ch<_num_dry_sounds;ch++)
      _dry_sound[ch] = (float*)calloc(sizeof(float),num_frames);

    _input_sound = (float**)(calloc(sizeof(float*),_num_inputs));
    for(int ch=0;ch<_num_inputs;ch++)
      _input_sound[ch] = (float*)calloc(sizeof(float),num_frames);

    _output_sound = (float**)(calloc(sizeof(float*),_num_outputs));
    for(int ch=0;ch<_num_outputs;ch++)
      _output_sound[ch] = (float*)calloc(sizeof(float),num_frames);
  }

  bool is_recursive(SoundProducer *start_producer){
    if(start_producer==this)
      return true;

    for(int ch=0;ch<_num_inputs;ch++){
      SoundProducerLink *link = (SoundProducerLink*)_input_producers[ch].next;
      while(link!=NULL){
        if(link->sound_producer->is_recursive(start_producer)==true)
          return true;
        
        link=(SoundProducerLink*)link->next;
      }
    }

    return false;
  }

  bool add_SoundProducerInput(SoundProducer *sound_producer, int sound_producer_ch, int ch){
    //fprintf(stderr,"*** this: %p. Adding input %p / %d,%d\n",this,sound_producer,sound_producer_ch,ch);

    if(sound_producer->is_recursive(this)==true){
      printf("Recursive tree detected\n");
      return false;
    }

    SoundProducerLink *link = new SoundProducerLink(); // created here
    link->sound_producer    = sound_producer;
    link->sound_producer_ch = sound_producer_ch;
    link->state = SoundProducerLink::FADING_IN;

    PLAYER_lock();{
      _input_producers[ch].add(link);
      MIXER_RT_set_bus_descendand_type_for_all_plugins();
    }PLAYER_unlock();

    return true;
  }

  void remove_SoundProducerInput(SoundProducer *sound_producer, int sound_producer_ch, int ch){
    //printf("**** Asking to remove connection\n");

    //fprintf(stderr,"*** this: %p. Removeing input %p / %d,%d\n",this,sound_producer,sound_producer_ch,ch);
    SoundProducerLink *link = (SoundProducerLink*)_input_producers[ch].next;
    while(link!=NULL){
      fprintf(stderr,"link: %p. link channel: %d\n",link,link->sound_producer_ch);
      if(link->sound_producer==sound_producer && link->sound_producer_ch==sound_producer_ch){

        PLAYER_lock();{
          link->state = SoundProducerLink::FADING_OUT;
        }PLAYER_unlock();

        RSEMAPHORE_wait(signal_from_RT,1);

        delete link; // deleted  here

        return;
      }
      link=(SoundProducerLink*)link->next;
    }

    fprintf(stderr,"huffda. link: %p. ch: %d\n",_input_producers[ch].next,ch);
    abort();
  }

  std::vector<SoundProducerLink *> get_all_links(void){
    std::vector<SoundProducerLink *> links;

    for(int ch=0;ch<_num_inputs;ch++){
      SoundProducerLink *link = (SoundProducerLink*)_input_producers[ch].next;
      while(link!=NULL){
        links.push_back(link);
        link=(SoundProducerLink*)link->next;
      }
    }

    return links;
  }

  // fade in 'input'
  void RT_crossfade_in(float *input, float *output, int num_frames){
    RT_fade_in(input, num_frames);
    RT_fade_out(output, num_frames);
          
    for(int i=0;i<num_frames;i++)
      output[i] += input[i];
  }

  // fade out 'input'
  void RT_crossfade_out(float *input, float *output, int num_frames){
    RT_fade_out(input, num_frames);
    RT_fade_in(output, num_frames);
          
    for(int i=0;i<num_frames;i++)
      output[i] += input[i];
  }

  void RT_apply_system_filter_apply(SystemFilter *filter, float **input, float **output, int num_channels, int num_frames){
    if(filter->plugins==NULL)
      if(num_channels==0){
        return;
      }else if(num_channels==1){
        float *s[2];
        float temp_ch2[num_frames];
        s[0] = input[0];
        s[1] = temp_ch2;
        memset(s[1],0,sizeof(float)*num_frames);
        COMPRESSOR_process(_plugin->compressor, s, s, num_frames);
        if(s[0] != output[0])
          memcpy(output[0],s[0],sizeof(float)*num_frames);
      }else{
        COMPRESSOR_process(_plugin->compressor, input, output, num_frames);
      }
    else
      for(int ch=0;ch<num_channels;ch++)
        PLUGIN_RT_process(filter->plugins[ch], 0, num_frames, &input[ch], &output[ch]);
  }

  // Quite chaotic with all the on/off is/was booleans.
  void RT_apply_system_filter(SystemFilter *filter, float **sound, int num_channels, int num_frames){
    if(filter->is_on==false && filter->was_on==false)
      return;

    {
      float *s[num_channels];
      float filter_sound[num_channels*num_frames];
      
      for(int ch=0;ch<num_channels;ch++)
        s[ch] = &filter_sound[ch*num_frames];
      
      if(filter->is_on==true){
        
        if(filter->was_off==true){ // fade in
          RT_apply_system_filter_apply(filter,sound,s,num_channels,num_frames);
          
          for(int ch=0;ch<num_channels;ch++)
            RT_crossfade_in(s[ch], sound[ch], num_frames);
          
          filter->was_off=false;
          
        }else{
          
          RT_apply_system_filter_apply(filter,sound,sound,num_channels,num_frames);
          
        }
        
        filter->was_on=true;
        
      }else if(filter->was_on==true){ // fade out.
        
        RT_apply_system_filter_apply(filter,sound,s,num_channels,num_frames);
        
        for(int ch=0;ch<num_channels;ch++)
          RT_crossfade_out(s[ch], sound[ch], num_frames);
        
        filter->was_on=false;
        filter->was_off=true;
      }

    }
  }

  void RT_process(int64_t time, int num_frames){
    if(_last_time == time)
      return;

    _last_time = time;


    PLUGIN_update_smooth_values(_plugin);

    // Gather sound data
    for(int ch=0;ch<_num_inputs;ch++){
      float *channel_target = _dry_sound[ch];
      SoundProducerLink *link = (SoundProducerLink*)_input_producers[ch].next;

      memset(channel_target, 0, sizeof(float)*num_frames);

      while(link!=NULL){
        SoundProducerLink *next = (SoundProducerLink*)link->next; // Store next poitner since the link can be removed here.

        SoundProducer *source_sound_producer = link->sound_producer;
        SoundPlugin *source_plugin = source_sound_producer->_plugin;

        float *input_producer_sound = source_sound_producer->RT_get_channel(time, num_frames, link->sound_producer_ch);

        // fade in
        if(link->state==SoundProducerLink::FADING_IN){
          float fade_sound[num_frames]; // When fading, 'input_producer_sound' will point to this array.
          memcpy(fade_sound,input_producer_sound, num_frames*sizeof(float));
          RT_fade_in(fade_sound, num_frames);
          input_producer_sound = &fade_sound[0];
          link->state=SoundProducerLink::STABLE;
        }

        // fade out
        else if(link->state==SoundProducerLink::FADING_OUT){
          float fade_sound[num_frames]; // When fading, 'input_producer_sound' will point to this array.
          memcpy(fade_sound,input_producer_sound, num_frames*sizeof(float));
          RT_fade_out(fade_sound, num_frames);
          input_producer_sound = &fade_sound[0];
          _input_producers[ch].remove(link);
          RSEMAPHORE_signal(signal_from_RT,1);
          MIXER_RT_set_bus_descendand_type_for_all_plugins();
        }

        // Apply volume and mix
        SMOOTH_mix_sounds(&source_plugin->output_volume, channel_target, input_producer_sound, num_frames);
        
        link = next;
      } // while(link!=NULL)
    }


    bool is_a_generator = _num_inputs==0;
    bool do_bypass      = _plugin->drywet.smoothing_is_necessary==false && _plugin->drywet.end_value==0.0f;


    if(is_a_generator)
      PLUGIN_RT_process(_plugin, time, num_frames, _input_sound, _dry_sound);


    // Input peaks
    {
      for(int ch=0;ch<_num_dry_sounds;ch++){
        float peak = do_bypass ? 0.0f : RT_get_max_val(_dry_sound[ch],num_frames) *  _plugin->input_volume.target_value;

        if(_plugin->input_volume_peak_values_for_chip!=NULL)
          _plugin->input_volume_peak_values_for_chip[ch] = peak;

        if(_plugin->input_volume_peak_values!=NULL)
          _plugin->input_volume_peak_values[ch] = peak;

        if(_plugin->system_volume_peak_values!=NULL) // the slider at the bottom bar.
          _plugin->system_volume_peak_values[ch] = peak;
      }
    }


    if(do_bypass){

      int num_channels = std::min(_num_dry_sounds,_num_outputs);
      for(int ch=0;ch<num_channels;ch++){
        memcpy(_output_sound[ch], _dry_sound[ch], num_frames*sizeof(float));
      }

    }else{ // do_bypass

      if(is_a_generator){

        // Apply input volume and fill output
        for(int ch=0;ch<_num_outputs;ch++)
          SMOOTH_copy_sound(&_plugin->input_volume, _output_sound[ch], _dry_sound[ch], num_frames);


      }else{

        // Apply input volume
        for(int ch=0;ch<_num_inputs;ch++)
          SMOOTH_copy_sound(&_plugin->input_volume, _input_sound[ch], _dry_sound[ch], num_frames);
        
        // Fill output
        {
          PLUGIN_RT_process(_plugin, time, num_frames, _input_sound, _output_sound);
        }

      }

      // compressor
      RT_apply_system_filter(&_plugin->comp,      _output_sound, _num_outputs, num_frames);

      // filters
      RT_apply_system_filter(&_plugin->lowshelf,  _output_sound, _num_outputs, num_frames);
      RT_apply_system_filter(&_plugin->eq1,       _output_sound, _num_outputs, num_frames);
      RT_apply_system_filter(&_plugin->eq2,       _output_sound, _num_outputs, num_frames);
      RT_apply_system_filter(&_plugin->highshelf, _output_sound, _num_outputs, num_frames);
      RT_apply_system_filter(&_plugin->lowpass,   _output_sound, _num_outputs, num_frames);
  
      // dry/wet
      RT_apply_dry_wet(_dry_sound, _num_dry_sounds, _output_sound, _num_outputs, num_frames, &_plugin->drywet);

    } // else do_bypass

    
    // Output pan
    SMOOTH_apply_pan(&_plugin->pan, _output_sound, _num_outputs, num_frames);

    // Right channel delay ("width")
    if(_num_outputs>1)
      RT_apply_system_filter(&_plugin->delay, &_output_sound[1], _num_outputs-1, num_frames);


    // Output peaks
    {
      for(int ch=0;ch<_num_outputs;ch++){
        float output_peak = RT_get_max_val(_output_sound[ch],num_frames);

        float output_volume_peak = output_peak * _plugin->volume;

        if(_plugin->volume_peak_values!=NULL)
          _plugin->volume_peak_values[ch] = output_volume_peak;
        
        if(_plugin->volume_peak_values_for_chip!=NULL)
          _plugin->volume_peak_values_for_chip[ch] = output_volume_peak;
        
        if(_plugin->output_volume_peak_values!=NULL)
          _plugin->output_volume_peak_values[ch] = output_peak * _plugin->output_volume.target_value;
        
        for(int bus=0;bus<2;bus++)
          if(_plugin->bus_volume_peak_values[bus]!=NULL)
            _plugin->bus_volume_peak_values[bus][ch] = output_peak * _plugin->bus_volume[bus].target_value;
      }
    }
  }

  float *RT_get_channel(int64_t time, int num_frames, int ret_ch){
    RT_process(time, num_frames);
    return _output_sound[ret_ch];
  }
};


SoundProducer *SP_create(SoundPlugin *plugin){
  static bool semaphore_inited=false;

  if(semaphore_inited==false){
    signal_from_RT = RSEMAPHORE_create(0);
    semaphore_inited=true;
  }

  return new SoundProducer(plugin, MIXER_get_buffer_size());
}

void SP_delete(SoundProducer *producer){
  if(strcmp(producer->_plugin->type->type_name,"Bus")){ // RT_process needs buses to always be alive.
    printf("Deleting \"%s\"\n",producer->_plugin->type->type_name);
    delete producer;
  }
}

// Returns false if the link could not be made. (i.e. the link was recursive)
bool SP_add_link(SoundProducer *target, int target_ch, SoundProducer *source, int source_ch){
  return target->add_SoundProducerInput(source,source_ch,target_ch);
}

void SP_remove_link(SoundProducer *target, int target_ch, SoundProducer *source, int source_ch){
  target->remove_SoundProducerInput(source,source_ch,target_ch);
}


#define STD_VECTOR_APPEND(a,b) a.insert(a.end(),b.begin(),b.end());

void SP_remove_all_links(std::vector<SoundProducer*> soundproducers){
  std::vector<SoundProducerLink *> links_to_delete;
  
  // Find links
  for(unsigned int i=0;i<soundproducers.size();i++){
    std::vector<SoundProducerLink *> links_to_delete_here = soundproducers.at(i)->get_all_links();
    STD_VECTOR_APPEND(links_to_delete, links_to_delete_here);
  }

  // Change state
  PLAYER_lock();{
    for(unsigned int i=0;i<links_to_delete.size();i++){
      links_to_delete.at(i)->state = SoundProducerLink::FADING_OUT;
    }
  }PLAYER_unlock();

  // Wait
  RSEMAPHORE_wait(signal_from_RT,links_to_delete.size());
 
  // Delete
  for(unsigned int i=0;i<links_to_delete.size();i++)
    delete links_to_delete.at(i);
}

void SP_RT_process(SoundProducer *producer, int64_t time, int num_frames){
  producer->RT_process(time, num_frames);
}

// This function is called from bus_type->RT_process.
void SP_RT_process_bus(float **outputs, int64_t time, int num_frames, int bus_num){
  DoublyLinkedList *sound_producer_list = MIXER_get_all_SoundProducers();

  memset(outputs[0],0,sizeof(float)*num_frames);
  memset(outputs[1],0,sizeof(float)*num_frames);

  while(sound_producer_list!=NULL){
    SoundProducer         *sp     = (SoundProducer*)sound_producer_list;
    SoundPlugin           *plugin = SP_get_plugin(sp);
    const SoundPluginType *type   = plugin->type;

    if(plugin->bus_descendant_type==IS_NOT_A_BUS_DESCENDANT){
      if(type->num_outputs==1){
        float *channel_data0 = sp->RT_get_channel(time, num_frames, 0);
        SMOOTH_mix_sounds(&plugin->bus_volume[bus_num], outputs[0], channel_data0, num_frames);
        SMOOTH_mix_sounds(&plugin->bus_volume[bus_num], outputs[1], channel_data0, num_frames);
      }else if(type->num_outputs>1){
        float *channel_data0 = sp->RT_get_channel(time, num_frames, 0);
        float *channel_data1 = sp->RT_get_channel(time, num_frames, 1);
        SMOOTH_mix_sounds(&plugin->bus_volume[bus_num], outputs[0], channel_data0, num_frames);
        SMOOTH_mix_sounds(&plugin->bus_volume[bus_num], outputs[1], channel_data1, num_frames);
      }
    }

    sound_producer_list = sound_producer_list->next;
  }
}

void SP_RT_set_bus_descendant_type_for_plugin(SoundProducer *producer){
  producer->RT_set_bus_descendant_type_for_plugin();
}

struct SoundPlugin *SP_get_plugin(SoundProducer *producer){
  return producer->_plugin;
}

SoundProducer *SP_get_SoundProducer(SoundPlugin *plugin){
  DoublyLinkedList *sound_producer_list = MIXER_get_all_SoundProducers();

  while(sound_producer_list!=NULL){
    SoundProducer *sp = (SoundProducer*)sound_producer_list;
    if(SP_get_plugin(sp)==plugin)
      return sp;

    sound_producer_list = sound_producer_list->next;
  }
  return NULL;
}

bool SP_is_plugin_running(SoundPlugin *plugin){
  return SP_get_SoundProducer(plugin)!=NULL;
}

void SP_set_buffer_size(SoundProducer *producer,int buffer_size){
  if(producer->_plugin->type->buffer_size_is_changed != NULL)
    producer->_plugin->type->buffer_size_is_changed(producer->_plugin,buffer_size);

  producer->free_sound_buffers();
  producer->allocate_sound_buffers(buffer_size);
}
