
#ifndef _RADIUM_AUDIO_MIXER_PROC_H
#define _RADIUM_AUDIO_MIXER_PROC_H 1

#ifdef MEMORY_DEBUG
extern LANGSPEC void PLAYER_memory_debug_wake_up(void);
#else
#define PLAYER_memory_debug_wake_up()
#endif

extern DEFINE_ATOMIC(bool, g_currently_processing_dsp);

extern LANGSPEC bool PLAYER_is_running(void);
  
extern LANGSPEC bool MIXER_start(void);
extern LANGSPEC void MIXER_stop(void);

extern LANGSPEC void MIXER_start_saving_soundfile(void);
extern LANGSPEC void MIXER_request_stop_saving_soundfile(void);

extern LANGSPEC void OS_InitAudioTiming(void);
extern LANGSPEC STime MIXER_get_block_delta_time(STime time);
extern LANGSPEC void MIXER_get_main_inputs(float **audio);
//extern LANGSPEC int64_t MIXER_get_time(void);
extern LANGSPEC int64_t MIXER_get_last_used_time(void);
extern LANGSPEC bool MIXER_fill_in_time_position(time_position_t *time_position);

#ifdef __cplusplus

extern void MIXER_add_SoundProducer(SoundProducer *sound_producer);

extern void MIXER_remove_SoundProducer(SoundProducer *sound_producer);

//extern void MIXER_get_buses(SoundProducer* &bus1, SoundProducer* &bus2);
  
#ifdef USE_QT4
#include "../common/Vector.hpp"
extern const radium::Vector<SoundProducer*> *MIXER_get_all_SoundProducers(void);
#endif

#endif // __cplusplus

extern LANGSPEC Buses MIXER_get_buses(void);
//extern LANGSPEC struct SoundProducer *MIXER_get_bus(int bus_num);

extern LANGSPEC struct Patch **RT_MIXER_get_all_click_patches(int *num_click_patches);

extern LANGSPEC float MIXER_get_sample_rate(void);

#ifdef USE_QT4
#include <QString>
namespace radium{
  static inline QString get_time_string(int64_t frames, bool include_centiseconds = true){
    return get_time_string((double)frames / MIXER_get_sample_rate(), include_centiseconds);
  }
}
#endif

extern LANGSPEC int MIXER_get_buffer_size(void);
extern LANGSPEC int MIXER_get_jack_block_size(void);

extern LANGSPEC struct SoundPlugin *MIXER_get_soundplugin(const char *type_name, const char *name);

extern LANGSPEC bool MIXER_someone_has_solo(void);

extern LANGSPEC void MIXER_called_regularly_by_main_thread(void);

extern LANGSPEC void MIXER_set_all_plugins_to_not_recording(void);

#endif
