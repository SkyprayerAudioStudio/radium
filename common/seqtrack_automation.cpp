/* Copyright 2016 Kjetil S. Matheussen

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
#include "hashmap_proc.h"
#include "patch_proc.h"
#include "seqtrack_proc.h"
#include "instruments_proc.h"
#include "Vector.hpp"
#include "../Qt/Qt_mix_colors.h"
#include "../Qt/Qt_colors_proc.h"
#include "SeqAutomation.hpp"
#include "../audio/SoundPlugin.h"
#include "../audio/SoundPlugin_proc.h"
#include "../audio/SoundProducer_proc.h"

#include "song_tempo_automation_proc.h"

#include <QPainter>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <QVector> // Shortening warning in the QVector header. Temporarily turned off by the surrounding pragmas.
#pragma clang diagnostic pop


#include "seqtrack_automation_proc.h"


namespace{

struct AutomationNode{
  double time; // seqtime format
  double value;
  int logtype;

};

static AutomationNode create_node(double seqtime, double value, int logtype){
  AutomationNode node = {
    .time = seqtime,
    .value = value,
    .logtype = logtype
  };
  return node;
}

static hash_t *get_node_state(const AutomationNode &node){
  hash_t *state = HASH_create(5);
  
  HASH_put_float(state, "seqtime", node.time);
  HASH_put_float(state, "value", node.value);
  HASH_put_int(state, "logtype", node.logtype);

  return state;
}

static AutomationNode create_node_from_state(hash_t *state){
  return create_node(HASH_get_float(state, "seqtime"),
                     HASH_get_float(state, "value"),
                     HASH_get_int32(state, "logtype"));
}


struct Automation{
  radium::SeqAutomation<AutomationNode> automation;
  struct Patch *patch; // I'm pretty sure we can use SoundPlugin directly now... I don't think patch->patchdata changes anymore.
  int effect_num = -1;
  double last_value = -1.0;
  QColor color;

  bool islegalnodenum(int nodenum){
    return nodenum>=0 && (nodenum<=automation.size()-1);
  }

  hash_t *get_state(void) const {
    SoundPlugin *plugin = (SoundPlugin*)patch->patchdata;

    hash_t *state = HASH_create(3);
    HASH_put_int(state, "patch", patch->id);
    HASH_put_chars(state, "effect_name", PLUGIN_get_effect_name(plugin, effect_num));
    HASH_put_hash(state, "automation", automation.get_state(get_node_state));
    return state;
  }

  Automation(struct Patch *patch, int effect_num)
    : patch(patch)
    , effect_num(effect_num)
  {
    SoundPlugin *plugin = (SoundPlugin*)patch->patchdata;
    R_ASSERT_RETURN_IF_FALSE(plugin!=NULL);

    color = get_qcolor(get_effect_color(plugin, effect_num));
  }

  Automation(hash_t *state){
    patch = PATCH_get_from_id(HASH_get_int(state, "patch"));
    automation.create_from_state(HASH_get_hash(state, "automation"), create_node_from_state);

    const char *effect_name = HASH_get_chars(state, "effect_name");

    SoundPlugin *plugin = (SoundPlugin*)patch->patchdata;
    R_ASSERT_RETURN_IF_FALSE(plugin!=NULL);

    const SoundPluginType *type=plugin->type;
    int i;
    
    for(i=0;i<type->num_effects+NUM_SYSTEM_EFFECTS;i++){
      if (!strcmp(PLUGIN_get_effect_name(plugin, i), effect_name))
        break;
    }
    
    if (i==type->num_effects+NUM_SYSTEM_EFFECTS)
      GFX_Message(NULL, "Sequencer automation: Could not find a effect named \"%s\" in %s/%s", effect_name, type->type_name, type->name);
    else
      effect_num = i;//HASH_get_int32(state, "effect_num");
  }
};

}


struct SeqtrackAutomation {
  
private:
  
  struct SeqTrack *_seqtrack; // Not used, but can be practical when debugging.

#if !defined(RELEASE)
  int magic = 918345; // Check that we are freeing the correct data.
#endif

  
public:

  radium::Vector<Automation*> _automations;
  
  SeqtrackAutomation(struct SeqTrack *seqtrack, const hash_t *state = NULL)
    :_seqtrack(seqtrack)
  {
    if (state != NULL) {
      int size = HASH_get_array_size(state);
      
      for(int i = 0 ; i < size ; i++){
        Automation *automation = new Automation(HASH_get_hash_at(state, "automation", i));
        if (automation->effect_num >= 0)
          _automations.push_back(automation);
      }
    }
  }

  ~SeqtrackAutomation(){
#if !defined(RELEASE)
    if (magic != 918345)
      abort();
#endif
    for(auto *automation : _automations)
      delete automation;
  }

  hash_t *get_state(void) const {
    hash_t *state = HASH_create(_automations.size());
    for(int i = 0 ; i < _automations.size() ; i++)
      HASH_put_hash_at(state, "automation", i, _automations.at(i)->get_state());
    return state;
  }

  int add_automation(struct Patch *patch, int effect_num, double seqtime1, double value1, int logtype, double seqtime2, double value2){
    Automation *automation = new Automation(patch, effect_num);

    automation->automation.add_node(create_node(seqtime1, value1, logtype));
    automation->automation.add_node(create_node(seqtime2, value2, logtype));

    {
      _automations.ensure_there_is_room_for_one_more_without_having_to_allocate_memory();
      {
        radium::PlayerLock lock;    
        _automations.push_back(automation);
      }  
      _automations.post_add();
    }
    
    return _automations.size()-1;
  }

private:
  
  Automation *find_automation(struct Patch *patch, int effect_num){
    for(auto *automation : _automations)
      if (automation->patch==patch && automation->effect_num==effect_num)
        return automation;

    RError("SeqtrackAutomation::find_automation: Could not find %s / %d", patch->name, effect_num);
    return NULL;    
  }
  
public:
  
  bool islegalautomation(int automationnum){
    return automationnum>=0 && (automationnum<=_automations.size()-1);
  }

  void remove_automation(Automation *automation){
    {
      radium::PlayerLock lock;    
      _automations.remove(automation);
    }

    delete automation;
  }

  void remove_automation(struct Patch *patch, int effect_num){
    Automation *automation = find_automation(patch, effect_num);
    if (automation==NULL)
      return;

    remove_automation(automation);
  }

};
 

struct SeqtrackAutomation *SEQTRACK_AUTOMATION_create(struct SeqTrack *seqtrack, const hash_t *automation_state){
  return new SeqtrackAutomation(seqtrack, automation_state);
}

void SEQTRACK_AUTOMATION_free(struct SeqtrackAutomation *seqtrackautomation){
  delete seqtrackautomation;
}

int SEQTRACK_AUTOMATION_add_automation(struct SeqtrackAutomation *seqtrackautomation, struct Patch *patch, int effect_num, double seqtime1, double value1, int logtype, double seqtime2, double value2){
  return seqtrackautomation->add_automation(patch, effect_num, seqtime1, value1, logtype, seqtime2, value2);
}

int SEQTRACK_AUTOMATION_get_num_automations(struct SeqtrackAutomation *seqtrackautomation){
  return seqtrackautomation->_automations.size();
}

struct Patch *SEQTRACK_AUTOMATION_get_patch(struct SeqtrackAutomation *seqtrackautomation, int automationnum){
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), (struct Patch*)get_audio_instrument()->patches.elements[0]);
  return seqtrackautomation->_automations[automationnum]->patch;
}

int SEQTRACK_AUTOMATION_get_effect_num(struct SeqtrackAutomation *seqtrackautomation, int automationnum){
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0);
  return seqtrackautomation->_automations[automationnum]->effect_num;
}

double SEQTRACK_AUTOMATION_get_value(struct SeqtrackAutomation *seqtrackautomation, int automationnum, int nodenum){
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0.5);

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE2(automation->islegalnodenum(nodenum), 0.5);

  return automation->automation.at(nodenum).value;
}

double SEQTRACK_AUTOMATION_get_seqtime(struct SeqtrackAutomation *seqtrackautomation, int automationnum, int nodenum){
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0.5);

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE2(automation->islegalnodenum(nodenum), 0.5);

  return automation->automation.at(nodenum).time;
}

int SEQTRACK_AUTOMATION_get_logtype(struct SeqtrackAutomation *seqtrackautomation, int automationnum, int nodenum){
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0.5);

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE2(automation->islegalnodenum(nodenum), 0.5);

  return automation->automation.at(nodenum).logtype;
}

int SEQTRACK_AUTOMATION_get_num_nodes(struct SeqtrackAutomation *seqtrackautomation, int automationnum){
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0.5);

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  return automation->automation.size();
}
  

int SEQTRACK_AUTOMATION_add_node(struct SeqtrackAutomation *seqtrackautomation, int automationnum, double seqtime, double value, int logtype){
  if (seqtime < 0)
    seqtime = 0;

  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0);

  struct Automation *automation = seqtrackautomation->_automations[automationnum];

  value = R_BOUNDARIES(0.0, value, 1.0);

  int ret = automation->automation.add_node(create_node(seqtime, value, logtype));
  
  SEQUENCER_update();
  
  return ret;
}
                              
void SEQTRACK_AUTOMATION_delete_node(struct SeqtrackAutomation *seqtrackautomation, int automationnum, int nodenum){
  R_ASSERT_RETURN_IF_FALSE(seqtrackautomation->islegalautomation(automationnum));

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE(automation->islegalnodenum(nodenum));

  if (automation->automation.size()<=2) {
    seqtrackautomation->remove_automation(automation);
  } else {
    automation->automation.delete_node(nodenum);
  }

  SEQUENCER_update();
}

void SEQTRACK_AUTOMATION_set_curr_node(struct SeqtrackAutomation *seqtrackautomation, int automationnum, int nodenum){
  R_ASSERT_RETURN_IF_FALSE(seqtrackautomation->islegalautomation(automationnum));

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE(automation->islegalnodenum(nodenum));

  if (automation->automation.get_curr_nodenum() != nodenum){
    automation->automation.set_curr_nodenum(nodenum);
    SEQUENCER_update();
  }
}

void SEQTRACK_AUTOMATION_cancel_curr_node(struct SeqtrackAutomation *seqtrackautomation, int automationnum){
  R_ASSERT_RETURN_IF_FALSE(seqtrackautomation->islegalautomation(automationnum));

  struct Automation *automation = seqtrackautomation->_automations[automationnum];

  automation->automation.set_curr_nodenum(-1);
  SEQUENCER_update();
}

void SEQTRACK_AUTOMATION_set_curr_automation(struct SeqtrackAutomation *seqtrackautomation, int automationnum){
  R_ASSERT_RETURN_IF_FALSE(seqtrackautomation->islegalautomation(automationnum));

  struct Automation *curr_automation = seqtrackautomation->_automations[automationnum];
  if (curr_automation->automation.do_paint_nodes())
    return;

  curr_automation->automation.set_do_paint_nodes(true);

  ALL_SEQTRACKS_FOR_EACH(){
    for(auto *automation : seqtrack->seqtrackautomation->_automations){
      if (automation != curr_automation)
        automation->automation.set_do_paint_nodes(false);
    }
  }END_ALL_SEQTRACKS_FOR_EACH;

  SEQUENCER_update();
}

void SEQTRACK_AUTOMATION_cancel_curr_automation(void){
  ALL_SEQTRACKS_FOR_EACH(){
    for(auto *automation : seqtrack->seqtrackautomation->_automations)
      automation->automation.set_do_paint_nodes(false);
  }END_ALL_SEQTRACKS_FOR_EACH;

  SEQUENCER_update();
}


int SEQTRACK_AUTOMATION_get_curr_automation(struct SeqtrackAutomation *seqtrackautomation){
  int ret = 0;

  for(auto *automation : seqtrackautomation->_automations){
    if (automation->automation.do_paint_nodes())
      return ret;

    ret++;
  }

  return -1;
}

void SEQTRACK_AUTOMATION_set(struct SeqtrackAutomation *seqtrackautomation, int automationnum, int nodenum, double seqtime, double value, int logtype){
  R_ASSERT_RETURN_IF_FALSE(seqtrackautomation->islegalautomation(automationnum));

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE(automation->islegalnodenum(nodenum));

  int size = automation->automation.size();

  const AutomationNode *prev = nodenum==0 ? NULL : &automation->automation.at(nodenum-1);
  AutomationNode node = automation->automation.at(nodenum);
  const AutomationNode *next = nodenum==size-1 ? NULL : &automation->automation.at(nodenum+1);

  double mintime = prev==NULL ? 0 : prev->time; //next==NULL ? R_MAX(R_MAX(node.time, seqtime), SONG_get_length()) : prev->time;
  double maxtime = next==NULL ? SONG_get_gfx_length()*MIXER_get_sample_rate() : next->time;

  //printf("mintime: %f, seqtime: %f, maxtime: %f\n",mintime,seqtime,maxtime);
  seqtime = R_BOUNDARIES(mintime, seqtime, maxtime);

  value = R_BOUNDARIES(0.0, value, 1.0);

  node.time = seqtime;
  node.value = value;
  node.logtype = logtype;

  automation->automation.replace_node(nodenum, node);

  SEQUENCER_update();
}


// Called from scheduler.c
void RT_SEQTRACK_AUTOMATION_called_per_block(struct SeqTrack *seqtrack){

  if (!is_playing() || pc->playtype!=PLAYSONG)
    return;

  int64_t seqtime = seqtrack->end_time; // use end_time instead of start_time to ensure automation is sent out before note start. (causes slightly inaccurate automation, but it probably doesn't matter)
    
  R_ASSERT_RETURN_IF_FALSE(seqtrack->seqtrackautomation!=NULL);

  for(auto *automation : seqtrack->seqtrackautomation->_automations){
    struct Patch *patch = automation->patch;
    int effect_num = automation->effect_num;

    R_ASSERT_RETURN_IF_FALSE(patch->instrument==get_audio_instrument());
    
    SoundPlugin *plugin = (SoundPlugin*) patch->patchdata;
    R_ASSERT_NON_RELEASE(plugin!=NULL);

    if (plugin!=NULL){
      
      double latency = RT_SP_get_input_latency(plugin->sp);
      if (latency!=0){
        struct SeqBlock *seqblock = seqtrack->curr_seqblock;
        if (seqblock != NULL){
          latency *= ATOMIC_DOUBLE_GET(seqblock->block->reltempo);
          latency = ceil(latency); // Ensure automation is sent out before note start. (probably not necessary)
        }
      }

      double value;
      if (automation->automation.RT_get_value(seqtime+latency, value)){
        if (value != automation->last_value){
          FX_when when;
          if (automation->last_value==-1.0)
            when = FX_start;
          else
            when = FX_middle;

          RT_PLUGIN_touch(plugin);

          plugin->automation_values[effect_num] = value;

          PLUGIN_set_effect_value(plugin,0,effect_num,value, PLUGIN_NONSTORED_TYPE, PLUGIN_DONT_STORE_VALUE, when);
          automation->last_value = value;
        }
      } else {
        if (automation->last_value != -1.0) {
          RT_PLUGIN_touch(plugin);

          PLUGIN_set_effect_value(plugin,0,effect_num, automation->last_value, PLUGIN_NONSTORED_TYPE, PLUGIN_DONT_STORE_VALUE, FX_end); // Send out the same value again, but with a different FX_when value. Slightly inefficient, but trying to predict when we need FX_end above is too complicated to be worth the effort.
          automation->last_value = -1.0;
        }
      }
    }
      
  }

}

void RT_SEQTRACK_AUTOMATION_called_when_player_stopped(void){
  ALL_SEQTRACKS_FOR_EACH(){

    for(auto *automation : seqtrack->seqtrackautomation->_automations)
      automation->last_value = -1.0;

  }END_ALL_SEQTRACKS_FOR_EACH;
}



hash_t *SEQTRACK_AUTOMATION_get_state(struct SeqtrackAutomation *seqtrackautomation){
  return seqtrackautomation->get_state();
}


static float get_node_x2(const struct SeqTrack *seqtrack, const AutomationNode &node, double start_time, double end_time, float x1, float x2){
  int64_t abstime = get_abstime_from_seqtime(seqtrack, NULL, node.time);

  return scale(abstime, start_time, end_time, x1, x2);
}

static float get_node_x(const AutomationNode &node, double start_time, double end_time, float x1, float x2, void *data){
  return get_node_x2((const struct SeqTrack*)data, node, start_time, end_time, x1, x2);
}

float SEQTRACK_AUTOMATION_get_node_x(struct SeqtrackAutomation *seqtrackautomation, struct SeqTrack *seqtrack, int automationnum, int nodenum){
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0);

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE2(automation->islegalnodenum(nodenum), 0);

  double start_time = SEQUENCER_get_visible_start_time();
  double end_time = SEQUENCER_get_visible_end_time();

  float x1 = SEQTRACK_get_x1(0);
  float x2 = SEQTRACK_get_x2(0);
  
  const AutomationNode &node = automation->automation.at(nodenum);

  return get_node_x2(seqtrack, node, start_time, end_time, x1, x2);
}

static float get_node_y(const AutomationNode &node, float y1, float y2){
  return scale(node.value, 0, 1, y2, y1);
}

float SEQTRACK_AUTOMATION_get_node_y(struct SeqtrackAutomation *seqtrackautomation, int seqtracknum, int automationnum, int nodenum){
  float y1 = SEQTRACK_get_y1(seqtracknum);
  float y2 = SEQTRACK_get_y2(seqtracknum);
  
  R_ASSERT_RETURN_IF_FALSE2(seqtrackautomation->islegalautomation(automationnum), 0);

  struct Automation *automation = seqtrackautomation->_automations[automationnum];
  R_ASSERT_RETURN_IF_FALSE2(automation->islegalnodenum(nodenum), 0);

  return get_node_y(automation->automation.at(nodenum), y1, y2);
}

void SEQTRACK_AUTOMATION_paint(QPainter *p, struct SeqTrack *seqtrack, float x1, float y1, float x2, float y2, double start_time, double end_time){
  
  for(auto *automation : seqtrack->seqtrackautomation->_automations)
    automation->automation.paint(p, x1, y1, x2, y2, start_time, end_time, automation->color, get_node_y, get_node_x, seqtrack);
}
