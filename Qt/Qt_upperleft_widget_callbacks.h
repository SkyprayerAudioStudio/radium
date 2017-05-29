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

#include "Qt_upperleft_widget.h"
#include "../common/undo_reltempomax_proc.h"
#include "../common/reallines_proc.h"
#include "../common/temponodes_proc.h"

#include "Rational.h"

#ifdef USE_QT5
#include <QStyleFactory>
#else
#include <QCleanlooksStyle>
#endif


extern EditorWidget *g_editor;

extern struct Root *root;


class Upperleft_widget : public QWidget, public Ui::Upperleft_widget {
  Q_OBJECT

 public:

  bool _is_initing;
  
 Upperleft_widget(QWidget *parent=NULL)
    : QWidget(parent)
  {
    _is_initing = true;
    
    setupUi(this);
    
#ifdef USE_QT5
#else
      setStyle(new QCleanlooksStyle);
#endif

      swing_onoff->setText("✔");
                            
    // Set up custom popup menues for the time widgets
    {
      bpm->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(bpm, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowBPMPopup(const QPoint&)));

      bpm_label->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(bpm_label, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowBPMPopup(const QPoint&)));

      lpb->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(lpb, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowLPBPopup(const QPoint&)));
      
      lpb_label->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(lpb_label, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowLPBPopup(const QPoint&)));
      
      signature->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(signature, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowSignaturePopup(const QPoint&)));

      signature_label->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(signature_label, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowSignaturePopup(const QPoint&)));

      SwingWidget->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(SwingWidget, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowSwingPopup(const QPoint&)));

      reltempomax->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(reltempomax, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowReltempoPopup(const QPoint&)));

      reltempomax_label->setContextMenuPolicy(Qt::CustomContextMenu);
      connect(reltempomax_label, SIGNAL(customContextMenuRequested(const QPoint&)),
              this, SLOT(ShowReltempoPopup(const QPoint&)));
    }

    _is_initing = false;
  }


  virtual void paintEvent( QPaintEvent *e ) override {
    radium::PaintEventTracker pet;
    
    //static int upcounter = 0;
    //printf("upperleft paintEvent %d\n",upcounter++);
    QPainter paint(this);
    //paint.fillRect(0,0,width(),height(),g_editor->colors[0]);
    paint.eraseRect(0,0,width(),height());
  }
  
  // called from the outside as well
  void updateWidgets(struct WBlocks *wblock){

    _is_initing = true;
        
    lz->setText(lz->getRational(wblock).toString());
    
    //printf("%d/%d (%s)\n",root->grid_numerator, root->grid_denominator, Rational(root->grid_numerator, root->grid_denominator).toString().toUtf8().constData());
    //getchar();
    
    grid->setText(Rational(root->grid_numerator, root->grid_denominator).toString());
    signature->setText(Rational(root->signature).toString());
    lpb->setValue(root->lpb);
    bpm->setValue(root->tempo);
    swing_onoff->setChecked(wblock->block->swing_enabled);

    // The bottom bar mirrors the lpb and bpm widgets.

    for(auto *bottom_bar : g_bottom_bars){
      bottom_bar->signature->setText(signature->text());
      bottom_bar->lpb->setValue(root->lpb);
      bottom_bar->bpm->setValue(root->tempo);
    }
    
    reltempomax->setValue(wblock->reltempomax);

    _is_initing = false;
  }

  void updateLayout(QWidget *w, int x1, int x2, int height){
    w->move(x1, 0);
    w->resize(x2-x1, height);

    QLayout *l = w->layout();

    l->setSpacing(2);
    l->setContentsMargins(0, 2, 2, 3);
  }
  
  // called from the outside
  void position(struct Tracker_Windows *window, struct WBlocks *wblock){

    GL_lock();{

      // upperleft lpb/bpm/reltempo show/hide
      if (window->show_lpb_track)
        LPBWidget->show();
      else
        LPBWidget->hide();
      
      if (window->show_bpm_track)
        BPMWidget->show();
      else
        BPMWidget->hide();
      
      if (window->show_swing_track)
        SwingWidget->show();
      else
        SwingWidget->hide();
      
      if (window->show_reltempo_track)
        ReltempoWidget->show();
      else
        ReltempoWidget->hide();
      
      // bottombar signature/lpb/bpm show/hide
      for(auto *bottom_bar : g_bottom_bars){
        if (window->show_signature_track)
          bottom_bar->SignatureWidget->hide();
        else
          bottom_bar->SignatureWidget->show();
        
        if (window->show_lpb_track)
          bottom_bar->LPBWidget->hide();
        else
          bottom_bar->LPBWidget->show();
        
        if (window->show_bpm_track)
          bottom_bar->BPMWidget->hide();
        else
          bottom_bar->BPMWidget->show();
        
        if (window->show_lpb_track && window->show_bpm_track)
          bottom_bar->tempo_line->hide();
      }
      
    } GL_unlock();
    
    int width = wblock->t.x1;
    int height = wblock->t.y1;
    
    //printf("resizing to %d - %d - %d - %d\n",x1,x2,x3,x4);

    resize(width,height);

    // Grid / tempocolor
    /////////////////////
    int x1 = 0; //wblock->tempocolorarea.x;
    int x2 = wblock->tempoTypearea.x;
    updateLayout(GridWidget, x1, x2, height);
    QMargins margins = GridWidget->layout()->contentsMargins();
    margins.setLeft(2);
    GridWidget->layout()->setContentsMargins(margins);
    
    // BPM
    //////////////////////////
    x1 = x2;
    x2 = wblock->lpbTypearea.x;
    updateLayout(BPMWidget, x1, x2, height);
    
    // LPB
    //////////////////////////
    x1 = x2;
    x2 = wblock->signaturearea.x;
    updateLayout(LPBWidget, x1, x2, height);
    
    // Signature
    ///////////////////////////
    x1 = x2;
    x2 = wblock->linenumarea.x;
    updateLayout(SignatureWidget, x1, x2, height);
    
    // LZ / linenumber(bars/beats)
    ///////////////////////////////
    x1 = x2;
    x2 = wblock->swingTypearea.x;
    updateLayout(lineZoomWidget, x1, x2, height);

    // Swing
    ///////////////////////////////
    x1 = x2;
    x2 = wblock->temponodearea.x;
    updateLayout(SwingWidget, x1, x2, height);

    // Reltempo (temponode)
    ///////////////////////////////
    x1 = x2;
    x2 = wblock->t.x1;
    updateLayout(ReltempoWidget, x1, x2, height);
    QMargins margins2 = ReltempoWidget->layout()->contentsMargins();
    margins2.setRight(1);
    ReltempoWidget->layout()->setContentsMargins(margins2);
    
  }

public slots:

  void ShowReltempoPopup(const QPoint& pos)
  {
    printf("GOTIT reltempo\n");
    if (simplePopupMenu("hide tempo multiplier track")==0)
      showHideReltempoTrack(-1);
  }

  void ShowBPMPopup(const QPoint& pos)
  {
    printf("GOTIT bpm\n");
    if (simplePopupMenu("hide BPM track")==0)
      showHideBPMTrack(-1);
  }

  void ShowLPBPopup(const QPoint& pos)
  {
    printf("GOTIT lpb\n");
    if (simplePopupMenu("hide LPB track")==0)
      showHideLPBTrack(-1);
  }

  void ShowSignaturePopup(const QPoint& pos)
  {
    printf("GOTIT signature\n");
    if (simplePopupMenu("hide time signature track")==0)
      showHideSignatureTrack(-1);
  }

  void ShowSwingPopup(const QPoint& pos)
  {
    printf("GOTIT swing\n");
    if (simplePopupMenu("hide swing track")==0)
      showHideSwingTrack(-1);
  }

  void on_swing_onoff_toggled(bool val){
    if (_is_initing==true)
      return;
    
    struct Tracker_Windows *window = root->song->tracker_windows;
    struct WBlocks *wblock = window->wblock;
    wblock->block->swing_enabled = val;
    TIME_block_swings_have_changed(wblock->block);
  }
    
  void on_lz_editingFinished(){
    if (_is_initing==true)
      return;

    // Also see lzqlineedit.h, where the 'wheelEvent' handler is implemented.

    printf("lz\n");
    struct Tracker_Windows *window = root->song->tracker_windows;
    struct WBlocks *wblock = window->wblock;

    Rational rational = create_rational_from_string(lz->text());

    int value = lz->getNumExpandLinesFromRational(rational);

    if (value==0) {
      
      updateWidgets(wblock);

    } else if (value != GetLineZoomBlock(wblock)) {
      
      LineZoomBlock(window,wblock,value);
      window->must_redraw = true;
      
    }
    
    set_editor_focus();      
  }

  void on_grid_editingFinished(){
    if (_is_initing==true)
      return;
    
    printf("grid\n");

    struct Tracker_Windows *window = root->song->tracker_windows;
    struct WBlocks *wblock = window->wblock;

    Rational rational = create_rational_from_string(grid->text());
    grid->pushValuesToRoot(rational);    

    updateWidgets(wblock);
    set_editor_focus();
  }

  void on_signature_editingFinished(){
    if (_is_initing==true)
      return;

    printf("signature\n");

    struct Tracker_Windows *window = root->song->tracker_windows;
    struct WBlocks *wblock = window->wblock;

    Rational rational = create_rational_from_string(signature->text());
    signature->pushValuesToRoot(rational);
    
    //setSignature(rational.numerator, rational.denominator);
    //printf("rational: %s. root: %d/%d",rational.toString().toUtf8().constData(),root->signature.numerator,root->signature.denominator);
    
    updateWidgets(wblock);
    set_editor_focus();
  }

  void on_lpb_editingFinished(){
    if (_is_initing==true)
      return;

    printf("lpb upperleft\n");
    setMainLPB(lpb->value());
    set_editor_focus();
  }

  void on_bpm_editingFinished(){
    if (_is_initing==true)
      return;

    printf("bpm upperleft\n");
    setMainBPM(bpm->value());
    set_editor_focus();
  }

  void on_reltempomax_editingFinished(){
    if (_is_initing==true)
      return;

    struct Tracker_Windows *window = root->song->tracker_windows;
    struct WBlocks *wblock = window->wblock;

    double new_value = reltempomax->value();
    if (new_value < 1.1){
      new_value = 1.1;
      reltempomax->setValue(new_value); // Setting the min value in the widget causes the widget to ignore trying to set a value less than the min value, not setting it to the min value.
    }
    
    if (fabs(new_value-wblock->reltempomax)<0.01) {
      set_editor_focus();
      return;
    }
    
    double highest = FindHighestTempoNodeVal(wblock->block) + 1.0f;

    if(highest > new_value)
      reltempomax->setValue(highest);
    
    ADD_UNDO(RelTempoMax(window,wblock));

    wblock->reltempomax=new_value;

    window->must_redraw = true;
    
    set_editor_focus();
  }
};

