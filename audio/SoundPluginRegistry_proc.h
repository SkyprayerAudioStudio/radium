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

#ifndef RADIUM_AUDIO_SOUNDPLUGIN_REGISTRY_PROC_H
#define RADIUM_AUDIO_SOUNDPLUGIN_REGISTRY_PROC_H

#ifdef __cplusplus
#  define LANGSPEC "C"
#else
#  define LANGSPEC
#endif

#ifdef USE_QT4

#include <QString>
#include <QVector>

#include "../common/Vector.hpp"

struct NumUsedPluginEntry{
  QString menu_text;
  QString container_name;
  QString type_name;
  QString name;
  int num_uses;    
};

// This struct is used a lot of places, so easiest thing is to do a "make clean" after changing it.
struct PluginMenuEntry{
  SoundPluginType *plugin_type;
  SoundPluginTypeContainer *plugin_type_container;
  QString level_up_name;

  NumUsedPluginEntry hepp;

  enum{
    IS_NORMAL = 0,
    IS_CONTAINER,
    IS_SEPARATOR,
    IS_LEVEL_UP,
    IS_LEVEL_DOWN,
    IS_LOAD_PRESET,
    IS_NUM_USED_PLUGIN
  } type;
  static PluginMenuEntry separator(){
    PluginMenuEntry entry;
    entry.type=IS_SEPARATOR;
    return entry;
  }
  static PluginMenuEntry load_preset(){
    PluginMenuEntry entry;
    entry.type=IS_LOAD_PRESET;
    return entry;
  }
  static PluginMenuEntry num_used_plugin(NumUsedPluginEntry hepp){
    PluginMenuEntry entry;
    entry.type=IS_NUM_USED_PLUGIN;
    entry.hepp = hepp;
    return entry;
  }  
  static PluginMenuEntry level_up(QString name){
    PluginMenuEntry entry;
    entry.type=IS_LEVEL_UP;
    entry.level_up_name=name;
    return entry;
  }
  static PluginMenuEntry level_down(){
    PluginMenuEntry entry;
    entry.type=IS_LEVEL_DOWN;
    return entry;
  }
  static PluginMenuEntry normal(SoundPluginType *plugin_type){
    PluginMenuEntry entry;
    entry.plugin_type = plugin_type;
    entry.type=IS_NORMAL;
    return entry;
  }
  static PluginMenuEntry container(SoundPluginTypeContainer *plugin_type_container){
    PluginMenuEntry entry;
    entry.plugin_type_container = plugin_type_container;
    entry.type=IS_CONTAINER;
    return entry;
  }
};

const QVector<PluginMenuEntry> PR_get_menu_entries(void); // Can not use radium::Vector if the contents requires copy constructor

void PR_add_menu_entry(PluginMenuEntry entry);

#endif // USE_QT4


extern LANGSPEC void PR_set_init_vst_first(void);
extern LANGSPEC void PR_set_init_ladspa_first(void);
extern LANGSPEC bool PR_is_initing_vst_first(void);

extern LANGSPEC void PR_add_plugin_type_no_menu(SoundPluginType *plugin_type);
extern LANGSPEC void PR_add_plugin_type(SoundPluginType *plugin_type);
extern LANGSPEC void PR_add_plugin_container(SoundPluginTypeContainer *container);
extern LANGSPEC int PR_get_num_plugin_types(void);
extern LANGSPEC SoundPluginType *PR_get_plugin_type(int num);
extern LANGSPEC SoundPluginType *PR_get_plugin_type_by_name(const char *container_name, const char *type_name, const char *plugin_name);
extern LANGSPEC void PR_init_plugin_types(void);

#endif
