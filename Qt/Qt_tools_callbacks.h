#include "../common/includepython.h"

#include <stdio.h>
#include <unistd.h>

#include <QScrollArea>
#include <QCheckBox>
#include <QTimer>
#include <QDialogButtonBox>

#include "../common/nsmtracker.h"

#include <FocusSniffers.h>

#include "../common/OS_visual_input.h"
#include "../common/Vector.hpp"
#include "../OpenGL/Widget_proc.h"
#include "../midi/midi_i_input_proc.h"

#include "../api/api_proc.h"

#include "Qt_MyQButton.h"

#include "Qt_tools.h"

namespace{

struct MidiLearnItem : public QWidget {
  Q_OBJECT;
  
  QCheckBox *enabled;
  QPushButton *delete_button;

public:

  MidiLearn *midi_learn;

  QLabel *source,*dest;
  
  MidiLearnItem(MidiLearn *midi_learn)
    : midi_learn(midi_learn)
  {
    auto *layout = new QHBoxLayout(this);

    enabled = new QCheckBox("Enabled", this);
    enabled->setChecked(true);
    layout->addWidget(enabled, 0);
    connect(enabled, SIGNAL(toggled(bool)), this, SLOT(enabled_toggled(bool)));
    
    delete_button = new QPushButton("Delete", this);
    layout->addWidget(delete_button, 0);
    connect(delete_button, SIGNAL(released()), this, SLOT(delete_released()));

    source = new QLabel("", this);
    dest = new QLabel("", this);
    
    layout->addWidget(source, 0);
    layout->addWidget(dest, 1);

    update_gui();
    
    setLayout(layout);  
  }

  void update_gui(){
    source->setText(midi_learn->get_source_info() + " => ");
    dest->setText(midi_learn->get_dest_info());
  }

public slots:

  void enabled_toggled(bool val){
    ATOMIC_SET(midi_learn->is_enabled, val);
    if (g_currpatch != NULL)
      ATOMIC_SET(g_currpatch->widget_needs_to_be_updated, true);
  }
  
  void delete_released(){
    midi_learn->delete_me();
  }
};
 
class MidiLearnPrefs : public RememberGeometryQDialog {
  Q_OBJECT;

  QLayout *main_layout;

  radium::VerticalScroll *vertical_scroll;

  radium::Vector<MidiLearnItem*> items;

  struct MyTimer : public QTimer {
    MidiLearnPrefs *parent;
    MyTimer(MidiLearnPrefs *parent)
      : parent(parent)
    {      
    }

    void timerEvent(QTimerEvent * e){
      if(g_radium_runs_custom_exec) return;

      if (parent->isVisible()){
        for(auto *item : parent->items)
          item->update_gui();
      }
    }
  };

  MyTimer timer;
  
public:
  MidiLearnPrefs(QWidget *parent=NULL)
    : RememberGeometryQDialog(parent, false)
    , timer(this)
  {
    main_layout = new QGridLayout(this);
    setLayout(main_layout);

    vertical_scroll = new radium::VerticalScroll(this);
    main_layout->addWidget(vertical_scroll);

    QDialogButtonBox *button_box = new QDialogButtonBox(this);
    button_box->setStandardButtons(QDialogButtonBox::Close);
    main_layout->addWidget(button_box);

    connect(button_box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(on_button_box_clicked(QAbstractButton*)));
        
    timer.setInterval(100);
    timer.start();
  }

  void add(MidiLearn *midi_learn){
    auto *gakk = new MidiLearnItem(midi_learn);
    items.push_back(gakk);
    vertical_scroll->addWidget(gakk);
  }
  
  void remove(MidiLearn *midi_learn){
    MidiLearnItem *item=NULL;
    for (auto item2 : items)
      if (item2->midi_learn==midi_learn){
        item = item2;
        break;
      }
    
    R_ASSERT_RETURN_IF_FALSE(item!=NULL);

    vertical_scroll->removeWidget(item);
    items.remove(item);

    delete item;
  }

public slots:
  void on_button_box_clicked(QAbstractButton *button){
    hide();
  }
};

 
class Tools : public RememberGeometryQDialog, public Ui::Tools {
  Q_OBJECT

 public:
  bool _initing;

  Tools(QWidget *parent=NULL)
    : RememberGeometryQDialog(parent, false)
  {
    _initing = true;

    setupUi(this);

    dyn_t quant_gui_dyn = evalScheme("(create-quantitize-gui)");
    if (quant_gui_dyn.type==INT_TYPE){
      
      int64_t quant_gui_num = quant_gui_dyn.int_number;
      if (quant_gui_num > 0){
        
        QWidget *quant_gui = API_gui_get_widget(quant_gui_num);
        if (quant_gui != NULL)
          quantization_layout->addWidget(quant_gui);
        
      }
      
    }
    
    updateWidgets();
 
    _initing = false;

    adjustSize();
  }

  virtual void showEvent(QShowEvent *event_) override {
    updateWidgets(); // background color is set back to high_background_color when changing colors in the preferences. This way at least the lighter colors come back when reopening the window.
    RememberGeometryQDialog::showEvent(event_);
  }
  
  void updateWidgets(){
    QPalette pal = palette();
    pal.setColor(QPalette::Background, get_qcolor(LOW_BACKGROUND_COLOR_NUM)); // Default HIGH_BACKGROUND_COLOR is very dark.
    setPalette(pal);
  }

public slots:

  void on_closeButton_clicked(){
    this->hide();
  }

  void on_make_track_monophonic_button_clicked(){
    makeTrackMonophonic(-1,-1,-1);
  }

  void on_split_track_button_clicked(){
    splitTrackIntoMonophonicTracks(-1,-1,-1);
  }
};

}

//#include "mQt_tools_callbacks.cpp"
