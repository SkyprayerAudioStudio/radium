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

#ifndef QT_INSTRUMENTS_PROC_H
#define QT_INSTRUMENTS_PROC_H

extern struct Patch *g_currpatch;

enum SizeType{
  SIZETYPE_NORMAL,
  SIZETYPE_HALF,
  SIZETYPE_FULL
};

extern LANGSPEC const char **get_ccnames(void);
  
extern LANGSPEC hash_t *create_instrument_widget_order_state(void);
extern LANGSPEC void recreate_instrument_widget_order_from_state(hash_t *state);

#ifdef USE_QT4
#include <QString>
extern QString request_load_preset_filename(void);
#endif

extern LANGSPEC const char *request_load_preset_encoded_filename(void);

extern LANGSPEC void InstrumentWidget_set_last_used_preset_filename(const wchar_t *wfilename);
  
extern LANGSPEC void InstrumentWidget_save_preset(struct Patch *patch);

extern LANGSPEC void InstrumentWidget_create_audio_instrument_widget(struct Patch *patch);
extern LANGSPEC void InstrumentWidget_delete(struct Patch *patch);

extern LANGSPEC void InstrumentWidget_prepare_for_deletion(struct Patch *patch);
extern LANGSPEC void GFX_update_instrument_widget(struct Patch *patch);
extern LANGSPEC void GFX_update_current_instrument_widget(void);

#ifdef USE_QT4
//static Audio_instrument_widget *get_audio_instrument_widget(struct Patch *patch);

#endif

extern LANGSPEC void AUDIOWIDGET_change_height(struct Patch *patch, enum SizeType type);
#if 0
extern LANGSPEC void AUDIOWIDGET_show_large(struct Patch *patch);
extern LANGSPEC void AUDIOWIDGET_show_small(struct Patch *patch);
#endif

struct SoundPlugin;
struct SoundPluginType;
struct SoundProducer;

extern LANGSPEC struct Patch *get_current_instruments_gui_patch(void);

#ifdef __cplusplus

namespace Ui{
  class Audio_instrument_widget;
};

Ui::Audio_instrument_widget *InstrumentWidget_get_audio_instrument_widget(struct Patch *patch);


class QWidget;
QWidget *createInstrumentsWidget(void);
#endif // __cplusplus

#endif
