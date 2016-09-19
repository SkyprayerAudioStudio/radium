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


#include "../audio/SoundPlugin_proc.h"
#include "../audio/SoundPluginRegistry_proc.h"

/*
class Pd_Plugin_widget;
class Audio_instrument_wigdet;
static Pd_Plugin_widget *AUDIOWIDGET_get_pd_plugin_widget(Audio_instrument_widget *audio_instrument_widget);
*/

#include "Qt_audio_instrument_widget.h"

#include "mQt_sample_requester_widget_callbacks.h"
#include "mQt_plugin_widget_callbacks.h"
#include "mQt_compressor_widget_callbacks.h"

#include "../mixergui/QM_chip.h"
#include "../audio/Bus_plugins_proc.h"


//extern void BottomBar_set_system_audio_instrument_widget_and_patch(Ui::Audio_instrument_widget *system_audio_instrument_widget, struct Patch *patch);

class Audio_instrument_widget : public QWidget, public Ui::Audio_instrument_widget{
  Q_OBJECT;

public:

  bool is_starting;
  
  struct Patch *_patch;

  Patch_widget *_patch_widget;
  Plugin_widget *_plugin_widget;
  
  Sample_requester_widget *_sample_requester_widget;
  Compressor_widget *_comp_widget;

  SizeType _size_type;
  SizeType _size_type_before_hidden;
  //bool _was_large_before_hidden;

  static void set_arrow_style(QWidget *arrow, bool set_size_policy=true){
    QPalette palette2;

    QBrush brush1(QColor(106, 104, 100, 255));
    brush1.setStyle(Qt::SolidPattern);

    QBrush brush3(QColor(85, 85, 0, 255));
    //QBrush brush3(QColor(0, 107, 156, 255));
    brush3.setStyle(Qt::SolidPattern);
    palette2.setBrush(QPalette::Active, QPalette::WindowText, brush3);
    palette2.setBrush(QPalette::Inactive, QPalette::WindowText, brush3);
    palette2.setBrush(QPalette::Disabled, QPalette::WindowText, brush1);
    palette2.setBrush(QPalette::Active, QPalette::Text, brush3);
    palette2.setBrush(QPalette::Inactive, QPalette::Text, brush3);
    palette2.setBrush(QPalette::Disabled, QPalette::Text, brush1);
    arrow->setPalette(palette2);
    //static_cast<QLabel*>(arrow)->setText("gakk");

#if 1
    if(set_size_policy){
      QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Fixed);
      sizePolicy4.setHorizontalStretch(0);
      sizePolicy4.setVerticalStretch(0);
      sizePolicy4.setHeightForWidth(arrow->sizePolicy().hasHeightForWidth());
      arrow->setSizePolicy(sizePolicy4);

      //arrow->setFrameShape(QFrame::Box);
    }
#endif
  }


 Audio_instrument_widget(QWidget *parent,struct Patch *patch)
    : QWidget(parent)
    , is_starting(true)
    , _patch(patch)
    , _plugin_widget(NULL)
    , _sample_requester_widget(NULL)
      //, _is_large(false)
    , _size_type(SIZETYPE_NORMAL)
    , _size_type_before_hidden(SIZETYPE_NORMAL)
  {

    setupUi(this);    

    /*
    {
      //QFontMetrics fm(font());
      _ab_checkbox_width = ab_reset_button->width(); //fm.width("A") * 4;
      ab_reset_button->setMinimumWidth(_ab_checkbox_width);
      ab_reset_button->setMaximumWidth(_ab_checkbox_width);
    }
    */
              
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;

    _patch_widget = new Patch_widget(this,patch);
    voiceBox_layout->insertWidget(0,_patch_widget, 1);
    //if(false && plugin->type->play_note==NULL)
    //  _patch_widget->voices_widget->setDisabled(true);

    SLIDERPAINTER_set_string(input_volume_slider->_painter, "In");
    SLIDERPAINTER_set_string(volume_slider->_painter, "Volume");
    SLIDERPAINTER_set_string(output_volume_slider->_painter, "Out");
    SLIDERPAINTER_set_string(panning_slider->_painter, "Panning");
    //velocity_slider->_painter, "Edit vel.";

    SLIDERPAINTER_set_string(drywet_slider->_painter, "Dry/Wet");
    SLIDERPAINTER_set_string(bus1_slider->_painter, "Reverb");
    SLIDERPAINTER_set_string(bus2_slider->_painter, "Chorus");
    SLIDERPAINTER_set_string(bus3_slider->_painter, "Aux 1");
    SLIDERPAINTER_set_string(bus4_slider->_painter, "Aux 2");
    SLIDERPAINTER_set_string(bus5_slider->_painter, "Aux 3");

    /*
    if(!strcmp(plugin->type->type_name,"Jack") && !strcmp(plugin->type->name,"System Out")){
      _i_am_system_out = true;
      BottomBar_set_system_audio_instrument_widget_and_patch(this, _patch);
    }
    */
    
#if 0
    if(plugin->type->num_inputs==0){
      drywet_slider->setDisabled(true);
      effects_onoff->setDisabled(true);
    }

    if(plugin->type->num_inputs==0){
      input_volume_slider->setDisabled(true);
      input_volume_onoff->setDisabled(true);
    }
#endif

    if(plugin->type->num_outputs==0){
      filters_widget->setDisabled(true);
      outputs_widget->setDisabled(true);
    }

    if(plugin->type->num_outputs<2){
      panning_onoff->setDisabled(true);
      panning_slider->setDisabled(true);
      rightdelay_onoff->setDisabled(true);
      rightdelay_slider->setDisabled(true);
    }

#if 0
    //instrument->effects_frame->addWidget(PluginWidget_create(NULL, plugin), 0, 3, 2, 1);
    _plugin_widget=PluginWidget_create(NULL, _patch);
    if(_plugin_widget->_param_widgets.size() > 0){
      delete(pipe_label);
      pipe_label = NULL;
    }
#else
    _plugin_widget = new Plugin_widget(this,_patch);

    delete(pipe_label);
    pipe_label = NULL;
#endif

    effects_layout->insertWidget(4,_plugin_widget);
    _plugin_widget->setVisible(plugin->show_controls_gui);
    spacer_holder->setVisible(!plugin->show_controls_gui);
    

    if(plugin->type==PR_get_plugin_type_by_name(NULL, "Sample Player","Sample Player") || plugin->type==PR_get_plugin_type_by_name(NULL, "Sample Player","Click") || plugin->type==PR_get_plugin_type_by_name(NULL, "FluidSynth","FluidSynth")){
      _sample_requester_widget = new Sample_requester_widget(this, _patch_widget->name_widget, _plugin_widget->sample_name_label, _patch);
      effects_layout->insertWidget(3,_sample_requester_widget);
      show_browser->setFixedWidth(browserArrow->width());

      /*
      QLabel *arrow = new QLabel("=>",this);
      arrow->setFrameShape(QFrame::Box);

      set_arrow_style(arrow);

      effects_layout->insertWidget(3, arrow, 0, Qt::AlignTop);
      arrow->show();
      */

      //MyQCheckBox *checkbox = new MyQCheckBox("hello", this);
      //effects_layout->

    } else
      browserWidget->hide();

    // Add compressor
    {
      _comp_widget = new Compressor_widget(patch, this);
      _comp_widget->setMinimumWidth(150);
      effects_layout->insertWidget(effects_layout->count()-2, _comp_widget);

      // these widgets are only used in the standalone version
      _comp_widget->load_button->hide();
      _comp_widget->save_button->hide();
      _comp_widget->radium_url->hide();
      _comp_widget->bypass->hide();

      GL_lock(); {
        _comp_widget->show();
        _comp_widget->setVisible(plugin->show_compressor_gui);
      }GL_unlock();
    }

    
    filters_widget->setVisible(plugin->show_equalizer_gui);


    // Adjust output widget widths
    {
      QFontMetrics fm(QApplication::font());
      //QRect r =fm.boundingRect(SLIDERPAINTER_get_string(_painter));
      int width = fm.width("Dry: 100% Wet: 0% Gak") + 10;
      lowpass_freq_slider->setMinimumWidth(width);
      highpass_freq_slider->setMinimumWidth(width);
      drywet_slider->setMinimumWidth(width);
    }


#if 0
    if(plugin->type->num_inputs==0){
      input_volume_layout->setDisabled(true);
    }
#endif


    // set the scroll bar itself to size 10.
    scrollArea->horizontalScrollBar()->setFixedHeight(10);
#ifdef USE_QT5
    //scrollArea->verticalScrollBar()->hide();
#endif
    
    updateWidgets();
    setupPeakAndAutomationStuff();

    is_starting = false;
  }

  void prepare_for_deletion(void){
    _comp_widget->prepare_for_deletion();
    _plugin_widget->prepare_for_deletion();
  }

  ~Audio_instrument_widget(){
    prepare_for_deletion();
  }
     
  MyQSlider *get_system_slider(int system_effect){
    switch(system_effect){

    case EFFNUM_INPUT_VOLUME:
      return input_volume_slider; 
    case EFFNUM_VOLUME:
      return volume_slider; 
    case EFFNUM_OUTPUT_VOLUME:
      return output_volume_slider;

    case EFFNUM_BUS1:
      return bus1_slider; 
    case EFFNUM_BUS2:
      return bus2_slider; 
    case EFFNUM_BUS3:
      return bus3_slider; 
    case EFFNUM_BUS4:
      return bus4_slider; 
    case EFFNUM_BUS5:
      return bus5_slider; 

    case EFFNUM_DRYWET:
      return drywet_slider; 

    case EFFNUM_PAN:
      return panning_slider; 

    case EFFNUM_LOWPASS_FREQ:
      return lowpass_freq_slider; 

    case EFFNUM_HIGHPASS_FREQ:
      return highpass_freq_slider; 

    case EFFNUM_EQ1_FREQ:
      return eq1_freq_slider; 
    case EFFNUM_EQ1_GAIN:
      return eq1_gain_slider; 

    case EFFNUM_EQ2_FREQ:
      return eq2_freq_slider; 
    case EFFNUM_EQ2_GAIN:
      return eq2_gain_slider; 

    case EFFNUM_LOWSHELF_FREQ:
      return lowshelf_freq_slider; 
    case EFFNUM_LOWSHELF_GAIN:
      return lowshelf_gain_slider; 

    case EFFNUM_HIGHSHELF_FREQ:
      return highshelf_freq_slider; 
    case EFFNUM_HIGHSHELF_GAIN:
      return highshelf_gain_slider; 

    case EFFNUM_DELAY_TIME:
      return rightdelay_slider; 

    default:
      return NULL;
    }
  }

  MyQCheckBox *get_system_checkbox(int system_effect){
    switch(system_effect){
    case EFFNUM_INPUT_VOLUME_ONOFF:
      return input_volume_onoff;
    case EFFNUM_VOLUME_ONOFF:
      return volume_onoff;
    case EFFNUM_OUTPUT_VOLUME_ONOFF:
      return output_volume_onoff;
    case EFFNUM_BUS1_ONOFF:
      return bus1_onoff;
    case EFFNUM_BUS2_ONOFF:
      return bus2_onoff;
    case EFFNUM_BUS3_ONOFF:
      return bus3_onoff;
    case EFFNUM_BUS4_ONOFF:
      return bus4_onoff;
    case EFFNUM_BUS5_ONOFF:
      return bus5_onoff;
    case EFFNUM_PAN_ONOFF:
      return panning_onoff;
    case EFFNUM_EFFECTS_ONOFF:
      return effects_onoff;
    case EFFNUM_LOWPASS_ONOFF:
      return lowpass_onoff;
    case EFFNUM_HIGHPASS_ONOFF:
      return highpass_onoff;

    case EFFNUM_EQ1_ONOFF:
      return eq1_onoff;
    case EFFNUM_EQ2_ONOFF:
      return eq2_onoff;
    case EFFNUM_LOWSHELF_ONOFF:
      return lowshelf_onoff;
    case EFFNUM_HIGHSHELF_ONOFF:
      return highshelf_onoff;

    case EFFNUM_DELAY_ONOFF:
      return rightdelay_onoff;

    default:
      return NULL;
    }
  }

  void updateSliderString(int system_effect){
    MyQSlider *slider = get_system_slider(system_effect);
    if(slider==NULL)
      return;

    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;
    int effect_num = type->num_effects + system_effect;

    SLIDERPAINTER_set_recording_color(slider->_painter, PLUGIN_is_recording_automation(plugin, effect_num));
    
    QString p = PLUGIN_has_midi_learn(plugin, effect_num) ? "*" : "";
          
    char buf[64]={0};
    PLUGIN_get_display_value_string(plugin, effect_num, buf, 64);
    if(system_effect==EFFNUM_DRYWET)
      SLIDERPAINTER_set_string(slider->_painter, p + QString(buf));
    else if(system_effect==EFFNUM_BUS1)
      SLIDERPAINTER_set_string(slider->_painter, p + QString(BUS_get_bus_name(0)) + ": " + QString(buf));
    else if(system_effect==EFFNUM_BUS2)
      SLIDERPAINTER_set_string(slider->_painter, p + QString(BUS_get_bus_name(1)) + ": " + QString(buf));
    else if(system_effect==EFFNUM_BUS3)
      SLIDERPAINTER_set_string(slider->_painter, p + QString(BUS_get_bus_name(2)) + ": " + QString(buf));
    else if(system_effect==EFFNUM_BUS4)
      SLIDERPAINTER_set_string(slider->_painter, p + QString(BUS_get_bus_name(3)) + ": " + QString(buf));
    else if(system_effect==EFFNUM_BUS5)
      SLIDERPAINTER_set_string(slider->_painter, p + QString(BUS_get_bus_name(4)) + ": " + QString(buf));
    else
      SLIDERPAINTER_set_string(slider->_painter, p + QString(PLUGIN_get_effect_name(plugin, effect_num)+strlen("System ")) + ": " + QString(buf));

    slider->update();
  }

  void updateSlider(int system_effect){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;
    int effect_num = type->num_effects + system_effect;

    MyQSlider* slider = get_system_slider(system_effect);
    float value = PLUGIN_get_effect_value(plugin, effect_num, VALUE_FROM_STORAGE);
    //printf("            %s, value: %f\n",PLUGIN_get_effect_name(plugin, effect_num), value);
    slider->setValue(value * 10000);

    updateSliderString(system_effect);
  }

  void updateChecked(QAbstractButton* checkwidget, int system_effect){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;

    int effect_num = type->num_effects + system_effect;

    bool val = PLUGIN_get_effect_value(plugin,effect_num,VALUE_FROM_STORAGE) > 0.5f;

    checkwidget->setChecked(val);

    show_compressor->setFixedWidth(controlsArrow->width());
    show_equalizer->setFixedWidth(controlsArrow->width());
    show_browser->setFixedWidth(browserArrow->width());
    show_controls->setFixedWidth(controlsArrow->width());

    if(system_effect==EFFNUM_INPUT_VOLUME_ONOFF)
      input_volume_slider->setEnabled(val);
    if(system_effect==EFFNUM_VOLUME_ONOFF)
      volume_slider->setEnabled(val);
    if(system_effect==EFFNUM_OUTPUT_VOLUME_ONOFF)
      output_volume_slider->setEnabled(val);
    if(system_effect==EFFNUM_BUS1_ONOFF)
      bus1_slider->setEnabled(val);
    if(system_effect==EFFNUM_BUS2_ONOFF)
      bus2_slider->setEnabled(val);
    if(system_effect==EFFNUM_BUS3_ONOFF)
      bus3_slider->setEnabled(val);
    if(system_effect==EFFNUM_BUS4_ONOFF)
      bus4_slider->setEnabled(val);
    if(system_effect==EFFNUM_BUS5_ONOFF)
      bus5_slider->setEnabled(val);

    if(system_effect==EFFNUM_LOWPASS_ONOFF)
      lowpass_freq_slider->setEnabled(val);
    if(system_effect==EFFNUM_HIGHPASS_ONOFF)
      highpass_freq_slider->setEnabled(val);
    if(system_effect==EFFNUM_EQ1_ONOFF){
      eq1_freq_slider->setEnabled(val);
      eq1_gain_slider->setEnabled(val);
    }
    if(system_effect==EFFNUM_EQ2_ONOFF){
      eq2_freq_slider->setEnabled(val);
      eq2_gain_slider->setEnabled(val);
    }
    if(system_effect==EFFNUM_LOWSHELF_ONOFF){
      lowshelf_freq_slider->setEnabled(val);
      lowshelf_gain_slider->setEnabled(val);
    }
    if(system_effect==EFFNUM_HIGHSHELF_ONOFF){
      highshelf_freq_slider->setEnabled(val);
      highshelf_gain_slider->setEnabled(val);
    }

    if(system_effect==EFFNUM_DELAY_ONOFF){
      rightdelay_slider->setEnabled(val);
    }
  }


  void calledRegularlyByParent(void){
    
    //printf("hello %p\n", this);
    
    //SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;

    if (_plugin_widget->isVisible())
      _plugin_widget->calledRegularlyByParent();
    
    callSystemSliderpainterUpdateCallbacks();
    
    if (_comp_widget->isVisible())
      _comp_widget->calledRegularlyByParent();

#ifdef USE_QT5
    bool is_visible = scrollArea->verticalScrollBar()->isVisible();
    if (is_visible)
      setMinimumHeight(height()+5);
#else
    adjustWidgetHeight();
#endif
  }

#ifndef USE_QT5
  // horror
  void adjustWidgetHeight(void){
    static bool shrinking = false;
    static int num_times_horizontal_is_not_visible;
    static bool can_shrink = true;
    static bool last_time_shrank = false;

    if (_size_type != SIZETYPE_NORMAL)
      return;
          
    bool is_visible = scrollArea->verticalScrollBar()->isVisible();
    
    if (scrollArea->horizontalScrollBar()->isVisible())
      num_times_horizontal_is_not_visible=0;
    else
      num_times_horizontal_is_not_visible++;
    
    if (is_visible){
      if (last_time_shrank)
        can_shrink = false;
      else
        can_shrink = true;
      setMinimumHeight(height()+1);
      GFX_ScheduleRedraw();
      //printf(" adjust 1\n");
      shrinking = false;
    } else if (is_visible==false && num_times_horizontal_is_not_visible>50 && can_shrink==true){
      shrinking = true;
    }
    
    if (shrinking){
      int old_size = minimumHeight();
      int new_size = old_size-1;
      if(new_size > 50){
        setMinimumHeight(new_size);
        GFX_ScheduleRedraw();
        //printf(" adjust 2\n");
      }
      last_time_shrank = true;
    }else
      last_time_shrank = false;
    
  }
#endif
  
  void callSystemSliderpainterUpdateCallbacks(void){
    
    for(int system_effect=0 ; system_effect<NUM_SYSTEM_EFFECTS ; system_effect++){
      MyQSlider *slider = get_system_slider(system_effect);
      if (slider != NULL) // effect can be a checkbox.
        slider->calledRegularlyByParent();
    }

  }

  void setupPeakAndAutomationStuff(void){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;

    // peaks
    
    int num_inputs = type->num_inputs;
    int num_outputs = type->num_outputs;

    if(num_outputs>0){
      SLIDERPAINTER_set_peak_value_pointers(volume_slider->_painter, num_outputs, plugin->volume_peak_values);

      SLIDERPAINTER_set_peak_value_pointers(output_volume_slider->_painter, num_outputs, plugin->output_volume_peak_values);

      SLIDERPAINTER_set_peak_value_pointers(bus1_slider->_painter,2, plugin->bus_volume_peak_values0);
      SLIDERPAINTER_set_peak_value_pointers(bus2_slider->_painter,2, plugin->bus_volume_peak_values1);
      SLIDERPAINTER_set_peak_value_pointers(bus3_slider->_painter,2, plugin->bus_volume_peak_values2);
      SLIDERPAINTER_set_peak_value_pointers(bus4_slider->_painter,2, plugin->bus_volume_peak_values3);
      SLIDERPAINTER_set_peak_value_pointers(bus5_slider->_painter,2, plugin->bus_volume_peak_values4);
    }

    if(num_inputs>0 || num_outputs>0){//plugin->input_volume_peak_values==NULL){
      if(num_inputs>0)
        SLIDERPAINTER_set_peak_value_pointers(input_volume_slider->_painter, num_inputs, plugin->input_volume_peak_values);
      else
        SLIDERPAINTER_set_peak_value_pointers(input_volume_slider->_painter, num_outputs, plugin->input_volume_peak_values);
    }

    // automation

    for(int system_effect=0 ; system_effect<NUM_SYSTEM_EFFECTS ; system_effect++){
      int effect_num = type->num_effects + system_effect;
      
      MyQSlider *slider = get_system_slider(system_effect);
      if (slider != NULL) // effect can be a checkbox.
        SLIDERPAINTER_set_automation_value_pointer(slider->_painter, get_effect_color(plugin, effect_num), &plugin->automation_values[effect_num]);
    }

#if 0
    PluginWidget *plugin_widget = NULL;

    if (_plugin_widget->_plugin_widget != NULL)
      plugin_widget = _plugin_widget->_plugin_widget;

#ifdef WITH_FAUST_DEV
    else if (_plugin_widget->_faust_plugin_widget != NULL)
      plugin_widget = _plugin_widget->_faust_plugin_widget->_plugin_widget;
#endif

    //if (plugin_widget != NULL)
    //  plugin_widget->set_automation_value_pointers(plugin);
#endif
    
    if (_plugin_widget->_pd_plugin_widget != NULL){

      for(unsigned int i=0; i<_plugin_widget->_pd_plugin_widget->_controllers.size(); i++) {
        Pd_Controller_widget *c = _plugin_widget->_pd_plugin_widget->_controllers[i];

        MyQSlider *slider = c->value_slider;
        if (slider != NULL){
          int effect_num = slider->_effect_num;
          SLIDERPAINTER_set_automation_value_pointer(slider->_painter, get_effect_color(plugin, effect_num), &plugin->automation_values[effect_num]);
        }
      }
    }
  }
  
  void updateWidgets(){
    set_arrow_style(controlsArrow, false);
    set_arrow_style(arrow2, false);
    set_arrow_style(arrow3);
    set_arrow_style(arrow4);
    set_arrow_style(arrow5);
    set_arrow_style(arrow6);
    set_arrow_style(arrow8);
    set_arrow_style(arrow9);
    set_arrow_style(arrow10);
    set_arrow_style(arrow7, false);
    set_arrow_style(browserArrow, false);
    if(pipe_label!=NULL)
      set_arrow_style(pipe_label,false);

    updateSlider(EFFNUM_BUS1);
    updateSlider(EFFNUM_BUS2);
    updateSlider(EFFNUM_BUS3);
    updateSlider(EFFNUM_BUS4);
    updateSlider(EFFNUM_BUS5);
    updateSlider(EFFNUM_DRYWET);
    updateSlider(EFFNUM_INPUT_VOLUME);
    updateSlider(EFFNUM_VOLUME);
    updateSlider(EFFNUM_OUTPUT_VOLUME);
    updateSlider(EFFNUM_PAN);

    updateSlider(EFFNUM_LOWPASS_FREQ);
    updateSlider(EFFNUM_HIGHPASS_FREQ);
    updateSlider(EFFNUM_EQ1_FREQ);
    updateSlider(EFFNUM_EQ1_GAIN);
    updateSlider(EFFNUM_EQ2_FREQ);
    updateSlider(EFFNUM_EQ2_GAIN);
    updateSlider(EFFNUM_LOWSHELF_FREQ);
    updateSlider(EFFNUM_LOWSHELF_GAIN);
    updateSlider(EFFNUM_HIGHSHELF_FREQ);
    updateSlider(EFFNUM_HIGHSHELF_GAIN);
    updateSlider(EFFNUM_DELAY_TIME);

    updateChecked(input_volume_onoff, EFFNUM_INPUT_VOLUME_ONOFF);
    updateChecked(volume_onoff, EFFNUM_VOLUME_ONOFF);
    updateChecked(output_volume_onoff, EFFNUM_OUTPUT_VOLUME_ONOFF);
    updateChecked(bus1_onoff, EFFNUM_BUS1_ONOFF);
    updateChecked(bus2_onoff, EFFNUM_BUS2_ONOFF);
    updateChecked(bus3_onoff, EFFNUM_BUS3_ONOFF);
    updateChecked(bus4_onoff, EFFNUM_BUS4_ONOFF);
    updateChecked(bus5_onoff, EFFNUM_BUS5_ONOFF);

    updateChecked(panning_onoff, EFFNUM_PAN_ONOFF);
    updateChecked(effects_onoff, EFFNUM_EFFECTS_ONOFF);

    updateChecked(lowpass_onoff, EFFNUM_LOWPASS_ONOFF);
    updateChecked(highpass_onoff, EFFNUM_HIGHPASS_ONOFF);
    updateChecked(eq1_onoff, EFFNUM_EQ1_ONOFF);
    updateChecked(eq2_onoff, EFFNUM_EQ2_ONOFF);
    updateChecked(lowshelf_onoff, EFFNUM_LOWSHELF_ONOFF);
    updateChecked(highshelf_onoff, EFFNUM_HIGHSHELF_ONOFF);
    updateChecked(rightdelay_onoff, EFFNUM_DELAY_ONOFF);

    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;

    for(int system_effect=EFFNUM_INPUT_VOLUME;system_effect<NUM_SYSTEM_EFFECTS;system_effect++){
      MyQSlider *slider = get_system_slider(system_effect);
      if(slider!=NULL){
        int effect_num = type->num_effects + system_effect;
        slider->_effect_num = effect_num;
        slider->_patch = _patch;
      }

      MyQCheckBox *checkbox = get_system_checkbox(system_effect);
      if(checkbox!=NULL){
        int effect_num = type->num_effects + system_effect;
        checkbox->_effect_num = effect_num;
        checkbox->_patch = _patch;
      }

      if(slider!=NULL && checkbox!=NULL)
        RError("weird");
    }

    update_all_ab_buttons();
    
    bool is_bus_provider = SP_get_bus_descendant_type(plugin->sp)==IS_BUS_PROVIDER;
    
    bus1_widget->setEnabled(is_bus_provider);
    bus2_widget->setEnabled(is_bus_provider);
    bus3_widget->setEnabled(is_bus_provider);
    bus4_widget->setEnabled(is_bus_provider);
    bus5_widget->setEnabled(is_bus_provider);

#if 0 //ndef RELEASE
    static int checknum;
    //    PLAYER_lock();
    fprintf(stderr,"\n\n     STARTING CHECK %d\n",checknum);
        
    const radium::Vector2<SoundProducer*> *org_all_sp = MIXER_get_all_SoundProducers();
    //void *elements1=org_all_sp->elements;
    int size = org_all_sp->size();
    QVector<SoundProducer*> sps;

    for(int n=0;n<size;n++)
      //    for (auto *sp : *all_sp)
      sps.push_back(org_all_sp->at(n));//[n]);//->elements[n]);

    for(int i = 0 ; i < 50000 ; i ++){
      const radium::Vector2<SoundProducer*> *all_sp = MIXER_get_all_SoundProducers();
      //void *elements2=all_sp->elements;
      R_ASSERT(all_sp->size() == size);
      
      for(int n=0;n<size;n++){
        SoundProducer *sp1 = sps[n];
        //SoundProducer *sp2 = all_sp->at(n);//[n];//((SoundProducer**)elements1)[n];//org_all_sp->elements[n];
        SoundProducer *sp2 = all_sp->elements[n];//at(n);//[n];//((SoundProducer**)elements1)[n];//org_all_sp->elements[n];
        if (sp1 != sp2){
          SoundPlugin *p1 = SP_get_plugin(sp1);
          SoundPlugin *p2 = SP_get_plugin(sp2);
          fprintf(stderr,"org_all_sp: %p, all_sp: %p, sp1: %p, sp2: %p. plug1: %s, plug2: %s\n",sp1,sp2,sp1,sp2,p1==NULL?"(null)":p1->patch->name,p2==NULL?"(null)":p2->patch->name);
          abort();
        }
        R_ASSERT(sps[n]==all_sp->at(n));//elements[n]);
      }
      
      //SP_get_SoundProducer(plugin);
    }
    fprintf(stderr,"\n\n     FINISHING CHECK %d\n",checknum++);
    //    PLAYER_unlock();
#endif
    
    //int num_inputs = type->num_inputs;
    int num_outputs = type->num_outputs;

    if(num_outputs>0){
      input_volume_layout->setEnabled(ATOMIC_GET(plugin->effects_are_on));
      if(plugin->type->num_inputs>0)
        _plugin_widget->setEnabled(ATOMIC_GET(plugin->effects_are_on));
      filters_widget->setEnabled(ATOMIC_GET(plugin->effects_are_on));
    }

    _comp_widget->update_gui();

    show_browser->setChecked(plugin->show_browser_gui);
    show_controls->setChecked(plugin->show_controls_gui);
    show_equalizer->setChecked(plugin->show_equalizer_gui);
    show_compressor->setChecked(plugin->show_compressor_gui);

    _patch_widget->updateWidgets();

    if(_sample_requester_widget != NULL){
      _sample_requester_widget->updateWidgets();
    }
  }

  void set_plugin_value(int sliderval, int system_effect){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;
    int effect_num = type->num_effects + system_effect;

    if (is_starting==false)
      PLUGIN_set_effect_value(plugin, -1, effect_num, sliderval/10000.0f, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single); // Don't need to lock player for setting system effects, I think. (it's probably better to let the plugins themselves decide whether to lock or not...)

    updateSliderString(system_effect);
  }

  
private:
  
  QVector<QWidget*> hidden_widgets;
  int height_before_large;

  // enable for faust and multiband
  void show_large(SizeType new_size_type){
    printf("show_large\n");

    R_ASSERT_RETURN_IF_FALSE(new_size_type!=_size_type);
    R_ASSERT_RETURN_IF_FALSE(new_size_type!=SIZETYPE_NORMAL);

    if (_size_type==SIZETYPE_NORMAL){
      hidden_widgets.clear();

      height_before_large = height();
      
      for (int i = 0; i < effects_layout->count(); ++i){
        QWidget *widget = effects_layout->itemAt(i)->widget();
        if (widget != NULL){
          if (widget != _plugin_widget && widget != _sample_requester_widget && widget->isVisible()){
            widget->hide();
            hidden_widgets.push_back(widget);
        }
        }
      }
    }

    if (new_size_type==SIZETYPE_HALF) {
      setMinimumHeight(g_main_window->height() / 2);
      setMaximumHeight(g_main_window->height() / 2);
    } else {
      setMinimumHeight(0);
      setMaximumHeight(g_main_window->height());
    }

    _size_type = new_size_type;
  }

  void show_small(void){
    printf("show_small\n");

    R_ASSERT_RETURN_IF_FALSE(_size_type!=SIZETYPE_NORMAL);
    
    _size_type=SIZETYPE_NORMAL;
    
    setMinimumHeight(10);
    setMaximumHeight(16777214);
    resize(width(), height_before_large);
    
    for (auto *widget : hidden_widgets)
      widget->show();
    
    hidden_widgets.clear();
  }

  void hide_non_instrument_widgets(void){
    /*
    GFX_HideEditor();
    GFX_HideMixer();
    GFX_PlayListWindowToBack();
    */
    struct Tracker_Windows *window = root->song->tracker_windows;
    EditorWidget *editor = static_cast<EditorWidget*>(window->os_visual.widget);
    editor->xsplitter->hide();
  }

  void show_non_instrument_widgets(void){
    struct Tracker_Windows *window = root->song->tracker_windows;
    EditorWidget *editor = static_cast<EditorWidget*>(window->os_visual.widget);
    editor->xsplitter->show();
    /*
    GFX_ShowEditor();
    GFX_ShowMixer();
    GFX_PlayListWindowToFront();
    */
  }

  
public:
  
  void change_height(SizeType new_size_type){
    if (new_size_type==_size_type)
      return;
    
    if (new_size_type==SIZETYPE_FULL)
      hide_non_instrument_widgets();    
    else if (_size_type == SIZETYPE_FULL)
      show_non_instrument_widgets();

    if (new_size_type==SIZETYPE_NORMAL)
      show_small();
    else
      show_large(new_size_type);
  }


  void hideEvent(QHideEvent * event){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    if (plugin != NULL)
      ATOMIC_SET(plugin->is_visible, false);
    
    _size_type_before_hidden = _size_type;
    
    if(_size_type != SIZETYPE_NORMAL)
      change_height(SIZETYPE_NORMAL);
      //show_small();  // If not, all instrument widgets will have large height.
  }
  
  void showEvent(QShowEvent * event){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    if (plugin != NULL)
      ATOMIC_SET(plugin->is_visible, true);
    
    if (_size_type_before_hidden != SIZETYPE_NORMAL)
      change_height(_size_type_before_hidden);
    //      show_large();
  }


  void update_all_ab_buttons(void){
    update_ab_button(ab0, 0);
    update_ab_button(ab1, 1);
    update_ab_button(ab2, 2);
    update_ab_button(ab3, 3);
    update_ab_button(ab4, 4);
    update_ab_button(ab5, 5);
    update_ab_button(ab6, 6);
    update_ab_button(ab7, 7);
    
    if(_plugin_widget != NULL)
      _plugin_widget->update_ab_buttons();
  }

  int _ab_checkbox_width = -1;

  void update_ab_button(QCheckBox *checkbox, int num){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;

    QString c = QString("ABCDEFGH"[num]);

    bool is_selected=plugin->curr_ab_num==num || plugin->ab_is_valid[num];
    
    if (is_selected)
      checkbox->setText(c+"*");
    else
      checkbox->setText(c);

    if (!is_selected && _ab_checkbox_width==-1){
      checkbox->adjustSize();
      _ab_checkbox_width = checkbox->width();
    }

    if (_ab_checkbox_width != -1){
      checkbox->setMinimumWidth(_ab_checkbox_width);
      checkbox->setMaximumWidth(_ab_checkbox_width);
    }
  }
  
  bool arrgh = false;
  
  void ab_pressed(QCheckBox *checkbox, int num, bool val){

    if (arrgh==false) {

      arrgh=true;
      
      SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;

      if (val==true){
        PLUGIN_change_ab(plugin, num);
        if (_plugin_widget != NULL){
          _plugin_widget->_ignore_checkbox_stateChanged = true; { // 'setChecked' should have an optional arguments whether to send signal to callbacks.
            
            _plugin_widget->ab_a->setChecked(num==0);
            _plugin_widget->ab_b->setChecked(num==1);
            /*          
                        ab0->setChecked(num==0);
                        ab1->setChecked(num==1);
                        ab2->setChecked(num==2);
                        ab3->setChecked(num==3);
                        ab4->setChecked(num==4);
                        ab5->setChecked(num==5);
                        ab6->setChecked(num==6);
                        ab7->setChecked(num==7);
            */
          }_plugin_widget->_ignore_checkbox_stateChanged = false;      
        }
        
      }

      update_ab_button(checkbox, num);

      arrgh=false;
    }
    
  }

public slots:

  void on_ab_reset_button_clicked(){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    PLUGIN_reset_ab(plugin);
    update_all_ab_buttons();
  }

  void on_ab0_toggled(bool val){ab_pressed(ab0, 0, val);}
  void on_ab1_toggled(bool val){ab_pressed(ab1, 1, val);}
  void on_ab2_toggled(bool val){ab_pressed(ab2, 2, val);}
  void on_ab3_toggled(bool val){ab_pressed(ab3, 3, val);}
  void on_ab4_toggled(bool val){ab_pressed(ab4, 4, val);}
  void on_ab5_toggled(bool val){ab_pressed(ab5, 5, val);}
  void on_ab6_toggled(bool val){ab_pressed(ab6, 6, val);}
  void on_ab7_toggled(bool val){ab_pressed(ab7, 7, val);}
  
  
  
#if 0
  void on_arrow1_toggled(bool val){
    _plugin_widget->setVisible(val);
  }

  void on_arrow2_toggled(bool val){
    filters_widget->setVisible(val);
  }

  void on_arrow3_toggled(bool val){
    outputs_widget1->setVisible(val);
    outputs_widget2->setVisible(val);
  }
#endif

  void on_show_browser_toggled(bool val){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;
    int effect_num = type->num_effects + EFFNUM_BROWSER_SHOW_GUI;
    PLUGIN_set_effect_value(plugin, -1, effect_num, val==true?1.0:0.0, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);
    if(_sample_requester_widget!=NULL)
      _sample_requester_widget->setVisible(val);
  }

  void on_show_controls_toggled(bool val){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;
    int effect_num = type->num_effects + EFFNUM_CONTROLS_SHOW_GUI;
    PLUGIN_set_effect_value(plugin, -1, effect_num, val==true?1.0:0.0, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);
    _plugin_widget->setVisible(val);
    spacer_holder->setVisible(!val);
  }

  void on_show_equalizer_toggled(bool val){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;
    int effect_num = type->num_effects + EFFNUM_EQ_SHOW_GUI;
    PLUGIN_set_effect_value(plugin, -1, effect_num, val==true?1.0:0.0, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);
    filters_widget->setVisible(val);
  }

  void on_show_compressor_toggled(bool val){
    SoundPlugin *plugin = (SoundPlugin*)_patch->patchdata;
    const SoundPluginType *type = plugin->type;
    int effect_num = type->num_effects + EFFNUM_COMP_SHOW_GUI;
    PLUGIN_set_effect_value(plugin, -1, effect_num, val==true?1.0:0.0, PLUGIN_NONSTORED_TYPE, PLUGIN_STORE_VALUE, FX_single);
    _comp_widget->setVisible(val);
  }

  void on_input_volume_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_INPUT_VOLUME_ONOFF);
    input_volume_slider->setEnabled(val);
  }

  void on_volume_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_VOLUME_ONOFF);
    volume_slider->setEnabled(val);
    CHIP_update((SoundPlugin*)_patch->patchdata);
  }

  void on_output_volume_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_OUTPUT_VOLUME_ONOFF);
    output_volume_slider->setEnabled(val);
  }

  void on_bus1_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_BUS1_ONOFF);
    bus1_slider->setEnabled(val);
  }
  void on_bus2_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_BUS2_ONOFF);
    bus2_slider->setEnabled(val);
  }
  void on_bus3_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_BUS3_ONOFF);
    bus3_slider->setEnabled(val);
  }
  void on_bus4_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_BUS4_ONOFF);
    bus4_slider->setEnabled(val);
  }
  void on_bus5_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_BUS5_ONOFF);
    bus5_slider->setEnabled(val);
  }

  void on_bus1_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_BUS1);
  }
  void on_bus2_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_BUS2);
  }
  void on_bus3_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_BUS3);
  }
  void on_bus4_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_BUS4);
  }
  void on_bus5_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_BUS5);
  }

  // effects onoff / dry_wet

  void on_effects_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_EFFECTS_ONOFF);
    drywet_slider->setEnabled(val);
    CHIP_update((SoundPlugin*)_patch->patchdata);
    updateWidgets();
  }

  void on_drywet_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_DRYWET);
  }

  void on_input_volume_slider_valueChanged(int val){
    if(_patch->patchdata != NULL) { // temp fix
      set_plugin_value(val, EFFNUM_INPUT_VOLUME);

      if (GFX_OS_patch_is_system_out(_patch))
        OS_GFX_SetVolume(val);

      CHIP_update((SoundPlugin*)_patch->patchdata);
    }
  }

  void on_volume_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_VOLUME);
    CHIP_update((SoundPlugin*)_patch->patchdata);
  }

  void on_output_volume_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_OUTPUT_VOLUME);
  }

  // lowpass 
  void on_lowpass_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_LOWPASS_ONOFF);
    lowpass_freq_slider->setEnabled(val);
  }

  void on_lowpass_freq_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_LOWPASS_FREQ);
  }

  // highpass 
  void on_highpass_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_HIGHPASS_ONOFF);
    highpass_freq_slider->setEnabled(val);
  }

  void on_highpass_freq_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_HIGHPASS_FREQ);
  }

  // eq1
  void on_eq1_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_EQ1_ONOFF);
    eq1_freq_slider->setEnabled(val);
    eq1_gain_slider->setEnabled(val);
  }

  void on_eq1_freq_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_EQ1_FREQ);
  }

  void on_eq1_gain_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_EQ1_GAIN);
  }

  // eq2
  void on_eq2_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_EQ2_ONOFF);
    eq2_freq_slider->setEnabled(val);
    eq2_gain_slider->setEnabled(val);
  }

  void on_eq2_freq_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_EQ2_FREQ);
  }

  void on_eq2_gain_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_EQ2_GAIN);
  }

  // lowshelf
  void on_lowshelf_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_LOWSHELF_ONOFF);
    lowshelf_freq_slider->setEnabled(val);
    lowshelf_gain_slider->setEnabled(val);
  }

  void on_lowshelf_freq_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_LOWSHELF_FREQ);
  }

  void on_lowshelf_gain_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_LOWSHELF_GAIN);
  }

  // highshelf
  void on_highshelf_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_HIGHSHELF_ONOFF);
    highshelf_freq_slider->setEnabled(val);
    highshelf_gain_slider->setEnabled(val);
  }

  void on_highshelf_freq_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_HIGHSHELF_FREQ);
  }

  void on_highshelf_gain_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_HIGHSHELF_GAIN);
  }

  // rightdelay
  void on_rightdelay_onoff_toggled(bool val){
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_DELAY_ONOFF);
    rightdelay_slider->setEnabled(val);
  }

  void on_rightdelay_slider_valueChanged(int val){
    set_plugin_value(val, EFFNUM_DELAY_TIME);
  }

#if 0
  void on_rightdelay_slider_sliderPressed(){
    rightdelay_slider->setEnabled(false);
    printf("Turning off slider\n");
  }

  void on_rightdelay_slider_sliderReleased(){
    rightdelay_slider->setEnabled(true);
    printf("Turning on slider\n");
  }
#endif

  // Volume

#if 0
  // Velocity

  void on_velocity_slider_valueChanged(int val)
  {
    _patch->standardvel = val*MAX_VELOCITY/10000;
  }
#endif


#if 0
  void on_velocity_onoff_stateChanged( int val)
  {
    if (val==Qt::Checked){
      velocity_slider->setEnabled(true);
      velocity_spin->setEnabled(true);;
    }else if(val==Qt::Unchecked){
      velocity_slider->setEnabled(false);
      velocity_spin->setEnabled(false);
    }
  }
#endif

  // Panning

  void on_panning_slider_valueChanged( int val)
  {
    set_plugin_value(val, EFFNUM_PAN);
  }

  void on_panning_onoff_toggled(bool val)
  {
    set_plugin_value(val==true ? 10000 : 0, EFFNUM_PAN_ONOFF);
    panning_slider->setEnabled(val);
  }
};

/*
static Pd_Plugin_widget *AUDIOWIDGET_get_pd_plugin_widget(Audio_instrument_widget *audio_instrument_widget){
  return audio_instrument_widget->_plugin_widget->_pd_plugin_widget;
}
*/

#ifdef WITH_FAUST_DEV
static Faust_Plugin_widget *AUDIOWIDGET_get_faust_plugin_widget(Audio_instrument_widget *audio_instrument_widget){
  return audio_instrument_widget->_plugin_widget->_faust_plugin_widget;
}
#endif

Sample_requester_widget *AUDIOWIDGET_get_sample_requester_widget(struct Patch *patch){
  return get_audio_instrument_widget(patch)->_sample_requester_widget;
}

void AUDIOWIDGET_change_height(struct Patch *patch, SizeType type){
  get_audio_instrument_widget(patch)->change_height(type);
}

void AUDIOWIDGET_set_ab(struct Patch *patch, int ab_num){
  auto *w = get_audio_instrument_widget(patch);

  switch(ab_num){
    case 0: w->ab0->setChecked(true); break;
    case 1: w->ab1->setChecked(true); break;
    case 2: w->ab2->setChecked(true); break;
    case 3: w->ab3->setChecked(true); break;
    case 4: w->ab4->setChecked(true); break;
    case 5: w->ab5->setChecked(true); break;
    case 6: w->ab6->setChecked(true); break;
    case 7: w->ab7->setChecked(true); break;
  }
}

void AUDIOWIDGET_redraw_ab(struct Patch *patch){
  get_audio_instrument_widget(patch)->update_all_ab_buttons();
}
  
#if 0
void AUDIOWIDGET_show_large(struct Patch *patch){
  get_audio_instrument_widget(patch)->show_large();
}

void AUDIOWIDGET_show_small(struct Patch *patch){
  get_audio_instrument_widget(patch)->show_small();
}
#endif

