#ifndef RADIUM_QT_HELPERS
#define RADIUM_QT_HELPERS

#include <QMessageBox>
#include <QMenu>
#include <QTimer>
#include <QTime>
#include <QMainWindow>
#include <QSplashScreen>
#include <QApplication>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGuiApplication>
#include <QPointer>
#include <QDesktopWidget>

#include "../OpenGL/Widget_proc.h"
#include "../common/keyboard_focus_proc.h"
#include "../common/visual_proc.h"
#include "../common/OS_system_proc.h"
#include "getqapplicationconstructorargs.hpp"

#include "../api/api_proc.h"


#define PUT_ON_TOP 0

extern bool g_radium_runs_custom_exec;

extern void set_editor_focus(void);

extern QVector<QWidget*> g_static_toplevel_widgets;

extern QMainWindow *g_main_window;
extern QSplashScreen *g_splashscreen;
extern QPointer<QWidget> g_current_parent_before_qmenu_opened; // Only valid if !g_curr_popup_qmenu.isNull()
extern QPointer<QMenu> g_curr_popup_qmenu;

typedef QPointer<QObject> IsAlive;


static QPoint getCentrePosition(QWidget *parent, int width, int height, QRect parentRect = QRect()){

  if (parentRect.isNull() || parentRect.isEmpty() || !parentRect.isValid()) {
    
    if (parent==NULL || parent->isVisible()==false)
      // Move to middle of screen instead.
      parentRect = QApplication::desktop()->availableGeometry();
    else
      parentRect = parent->window()->geometry();
    
  }
  
  int x = parentRect.x()+parentRect.width()/2-width/2;
  int y = parentRect.y()+parentRect.height()/2-height/2;
  //printf("w: %d, h: %d\n",width,height);

  return QPoint(R_MAX(20, x), R_MAX(20, y));
}

static inline void moveWindowToCentre(QWidget *widget, QRect parentRect = QRect()){
  int width = R_MAX(widget->width(), 100);
  int height = R_MAX(widget->height(), 50);
  QPoint point = getCentrePosition(widget->parentWidget(), width, height, parentRect);

  widget->move(point);
}

static inline void adjustSizeAndMoveWindowToCentre(QWidget *widget, QRect parentRect = QRect()){
  widget->adjustSize();
  widget->updateGeometry();
  moveWindowToCentre(widget, parentRect);
}

static bool can_widget_be_parent_questionmark(QWidget *w, bool is_going_to_run_custom_exec){
  if (w==NULL)
    return false;
  /*
  if (w==g_curr_popup_qmenu)
    return false;
  if (w->windowFlags() & Qt::Popup)
    return false;
  if (w->windowFlags() & Qt::ToolTip)
    return false;
  */

  // Should be safe. When running custom exec, a widget can not be deleted until exec() is finished.
  if(is_going_to_run_custom_exec==true)
    return true;
  
  return true;
}

// Can only return a widget that is a member of g_static_toplevel_widgets.
static inline QWidget *get_current_parent(bool is_going_to_run_custom_exec, bool may_return_current_parent_before_qmenu_opened = true){

  if (may_return_current_parent_before_qmenu_opened && !g_curr_popup_qmenu.isNull() && !g_current_parent_before_qmenu_opened.isNull()){
    //printf("1111 %p\n", g_current_parent_before_qmenu_opened.data());
    return g_current_parent_before_qmenu_opened;
  }

  QWidget *ret = QApplication::activeModalWidget();
  //printf("2222 %p\n", ret);
  if (can_widget_be_parent_questionmark(ret, is_going_to_run_custom_exec)){
    return ret;
  }

  ret = QApplication::focusWidget();
  //printf("333 %p\n", ret);
  if (can_widget_be_parent_questionmark(ret, is_going_to_run_custom_exec)){
    return ret;
  }

  ret = QApplication::activePopupWidget();
  //printf("333555 %p\n", ret);
  if (can_widget_be_parent_questionmark(ret, is_going_to_run_custom_exec)){
    return ret;
  }

  ret = QApplication::activeWindow();
  //printf("444 %p\n", ret);
  if (can_widget_be_parent_questionmark(ret, is_going_to_run_custom_exec)){
    return ret;
  }

  /*
  QWidget *mixer_strips_widget = MIXERSTRIPS_get_curr_widget();
  printf("555 %p\n", ret);
  if (mixer_strips_widget!=NULL){
    return mixer_strips_widget;
  }
  */
  
  ret = QApplication::widgetAt(QCursor::pos());
  //printf("666 %p\n", ret);
  if (g_static_toplevel_widgets.contains(ret)){
    return ret;
  }

  //printf("777\n");
  return g_main_window;
    
    /*
    if (curr_under != NULL)
      return curr_under;
    else if (QApplication::activeWindow()!=NULL)
      return QApplication::activeWindow();
    else
      return g_main_window;
    */
}

#define DEFAULT_WINDOW_FLAGS (Qt::CustomizeWindowHint | Qt::WindowFullscreenButtonHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint | Qt::WindowStaysOnTopHint)

// Returns true if modality is turned on when 'is_modal'==false.
static inline bool set_window_parent_andor_flags(QWidget *window, QWidget *parent, bool is_modal, bool only_set_flags){

//  #if defined(FOR_MACOSX)
#if 1
  // Although these hacks are only needed on OSX, it's probably best to let the program behave similarly on all platforms. (this behavior is not that bad)
  // Besides, it may be that setParent isn't always working properly on windows and linux either, for all I know.
  const bool set_parent_working_properly = false;
#else
  const bool set_parent_working_properly = true;
#endif

  Qt::WindowFlags f = Qt::Window | DEFAULT_WINDOW_FLAGS;
  bool force_modal = false;

  if (!set_parent_working_properly){
    if (parent==g_main_window) {

      // On OSX, you can't create an "on-top-of hierarchy" by setting the windows parent. But for level 1, we can work around this by setting the Qt::Tool flag.
      f = Qt::Window | Qt::Tool | DEFAULT_WINDOW_FLAGS;
      
      // Qt::Tool windows dissapear on OSX if the application is not active. (At least according to Qt documentation. I haven't tested it.)
      window->setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
      
    } else
      force_modal=true; // Qt::Tool doesn't work for levels larger than 1 (it doesn't work if the parent is a Qt::Tool window), so we work around it by using modal windows. Modal windows seems to always be on top of parent window.
  }
  
  if (only_set_flags)
    window->setWindowFlags(f);
  else
    window->setParent(parent, f);
 
  if (force_modal || is_modal) {

    if (window->windowModality()!=Qt::ApplicationModal)
      window->setWindowModality(Qt::ApplicationModal);

    return !is_modal;

  } else {

    if (!is_modal && window->windowModality()!=Qt::NonModal) // We may have forcefully turned on modality in a previous call. Turn it off now.
      window->setWindowModality(Qt::NonModal);

    return false;
    
  }
}

// Returns true if modality is turned on when 'is_modal'==false.
static inline bool set_window_parent(QWidget *window, QWidget *parent, bool is_modal){
  return set_window_parent_andor_flags(window, parent, is_modal, false);
}

// Returns true if modality is turned on when 'is_modal'==false.
static inline bool set_window_flags(QWidget *window, bool is_modal){
  return set_window_parent_andor_flags(window, window->parentWidget(), is_modal, true);
}

namespace radium{
  struct ASMTimer : public QTimer{
    QTime time;
    bool left_mouse_is_down = false;
    
    ASMTimer(QWidget *parent_)
      :QTimer(parent_)
    {
      time.start();
      setInterval(10);
    }

    void timerEvent ( QTimerEvent * e ){
      if (QGuiApplication::mouseButtons()==Qt::LeftButton)
        left_mouse_is_down = true;
      else if (left_mouse_is_down){
        left_mouse_is_down = false;
        time.restart();
      }
        
      //printf("  MOUSE DOWN: %d\n", QGuiApplication::mouseButtons()==Qt::LeftButton);
    }

    bool mouseWasDown(void){
      if(left_mouse_is_down==true)
        return true;
      if (time.elapsed() < 500)
        return true;
      return false;
    }
  };

  class VerticalScroll : public QScrollArea {
    //    Q_OBJECT;
    
  public:

    QVBoxLayout *layout;    
    
    VerticalScroll(QWidget *parent_)
      :QScrollArea(parent_)
    {
      setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      setWidgetResizable(true);
      
      QWidget *contents = new QWidget(this);
      
      layout = new QVBoxLayout(contents);
      layout->setSpacing(1);

      contents->setLayout(layout);
      
      setWidget(contents);    
    }
    
    void addWidget(QWidget *widget_){
      layout->addWidget(widget_);
    }
    
    void removeWidget(QWidget *widget_){
      layout->removeWidget(widget_);
    }
  };


}

extern bool g_qt_is_painting;
extern const char *g_qt_is_painting_where;

namespace radium{
  struct PaintEventTracker{
    PaintEventTracker(const char *where){
      R_ASSERT_NON_RELEASE(THREADING_is_main_thread());
      R_ASSERT_NON_RELEASE(g_qt_is_painting==false);
      g_qt_is_painting_where = where;
      g_qt_is_painting=true;
    }
    ~PaintEventTracker(){
      R_ASSERT_NON_RELEASE(g_qt_is_painting==true);
      g_qt_is_painting=false;
    }
  };
}

#define TRACK_PAINT() radium::PaintEventTracker radium_internal_pet(CR_FORMATEVENT(""))


// Why doesn't Qt provide this one?
namespace{
template <typename T>
struct ScopedQPointer : public QPointer<T>{
  T *_widget;
  ScopedQPointer(T *widget)
    : QPointer<T>(widget)
    , _widget(widget)
  {}
  ~ScopedQPointer(){
    printf("Deleting scoped pointer widget %p\n",_widget);
    delete _widget;
  }
};
/*
template <typename T>
struct ScopedQPointer {

  QPointer<T> _box;
  
  MyQPointer(T *t){
    _box = t;
    //printf("MyQPointer created %p for %p %p %p\n", this, _box, _box.data(), t);
  }

  ~MyQPointer(){    
    //printf("MyQPointer %p deleted (_box: %p %p)\n", this, _box, _box.data());
    delete data();
  }

  inline T* data() const
  {
    return _box.data();
  }
  inline T* operator->() const
  { return data(); }
  inline T& operator*() const
  { return *data(); }
  inline operator T*() const
  { return data(); }
};
*/
}


struct MyQMessageBox : public QMessageBox {
  bool _splashscreen_visible;

  // Prevent stack allocation. Not sure, but I think get_current_parent() could return something that could be deleted while calling exec.
  MyQMessageBox(const MyQMessageBox &) = delete;
  MyQMessageBox & operator=(const MyQMessageBox &) = delete;
  MyQMessageBox(MyQMessageBox &&) = delete;
  MyQMessageBox & operator=(MyQMessageBox &&) = delete;

  //
  // !!! parent must/should be g_main_window unless we use ScopedQPointer !!!
  //
  static MyQMessageBox *create(bool is_going_to_run_custom_exec, QWidget *parent = NULL){
    return new MyQMessageBox(is_going_to_run_custom_exec, parent);
  }
  
 private:  
    
  MyQMessageBox(bool is_going_to_run_custom_exec, QWidget *parent_ = NULL)
    : QMessageBox(parent_!=NULL ? parent_ : get_current_parent(is_going_to_run_custom_exec))
  {
    //printf("            PAERENT: %p. visible: %d\n",parent(),dynamic_cast<QWidget*>(parent())==NULL ? 0 : dynamic_cast<QWidget*>(parent())->isVisible());
    if(dynamic_cast<QWidget*>(parent())==NULL || dynamic_cast<QWidget*>(parent())->isVisible()==false){
      //setWindowModality(Qt::ApplicationModal);
      //setWindowFlags(Qt::Popup | Qt::WindowStaysOnTopHint);
      //setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
      setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::MSWindowsFixedSizeDialogHint);
      raise();
      activateWindow();
    } else {
      set_window_flags(this, true);
    }
    adjustSizeAndMoveWindowToCentre(this);
  }

 public:
  
  ~MyQMessageBox(){
    printf("MyQMessageBox %p deleted\n", this);
  }
  
  void showEvent(QShowEvent *event_){
    _splashscreen_visible = g_splashscreen!=NULL && g_splashscreen->isVisible();

    if (_splashscreen_visible)
      g_splashscreen->hide();

    QMessageBox::showEvent(event_);
  }
  
  void hideEvent(QHideEvent *event_) {
    if (_splashscreen_visible && g_splashscreen!=NULL)
      g_splashscreen->show();

    QMessageBox::hideEvent(event_);
  }
  
};


static inline void setUpdatesEnabledRecursively(QWidget *widget, bool doit){
  if (widget != NULL){
    widget->setUpdatesEnabled(doit);
    
    for(auto *c : widget->children()){
      QWidget *w = dynamic_cast<QWidget*>(c);      
      if (w && w->isWindow()==false)
        setUpdatesEnabledRecursively(w, doit);
    }
  }
}

namespace{
  struct PauseUpdatesTimer : public QTimer{
    QWidget *_w;
    QTime _time;
    int _ms;
    bool _has_done_step1 = false;
    
    PauseUpdatesTimer(QWidget *parent, int ms)
      : _w(parent)
      , _ms(ms)
    {
      //setSingleShot(true);
      _time.start();
      setInterval(15);
      start();
    }

    void timerEvent(QTimerEvent *e) override{
      if (_time.elapsed() >= _ms){ // singleshot is messy since we might get deleted at any time.
        //printf("TEIMERERINE EVENT\n");
        if (_has_done_step1==false && _w->isVisible()==false){
          setUpdatesEnabledRecursively(_w, true);
          _ms *= 2;
          _has_done_step1=true;
        } else {
          setUpdatesEnabledRecursively(_w, true);
          _w->show();
          stop();
          delete this;
        }
      }
    }
  };
}

static inline void pauseUpdates(QWidget *w, int ms = 50){
  setUpdatesEnabledRecursively(w, false);
  new PauseUpdatesTimer(w, ms);
}

static inline void updateWidgetRecursively(QObject *object){
  if (object != NULL){

    QWidget *w = dynamic_cast<QWidget*>(object);
    if (w!=NULL){
      //printf("Updating %p. (%s)\n", w, w->metaObject()->className());
      w->update();
    }
    
    for(auto *c : object->children())
      updateWidgetRecursively(c);
  }
}

// uncomment if needed.
//extern QByteArray g_filedialog_geometry;

namespace{

  /*
    Qt makes it _almost_ impossible to remember geometry of windows (both qdialog and qwidget) without adding a timer that monitors what's happening and tries to do the right thing.
    (and we most certainly don't want to that)

    The problem is that Qt always opens the windows at the original position when calling setVisible(true) or show(). There's no way to override that. It would be the most
    natural thing in the world to override, but there is no way. The only way to open at the original position is to remember geometry when hiding, and restore when showing.
    Unfortunatly things becomes very complicated since it's unclear (probably also for those who develops Qt) when the widgets are actually shown and hidden.

    However, I have found that the following seems to work (at least for Qt 5.5.1 on Linux with FVWM2):

    To use it, override setVisible and hideEvent like this:

    void setVisible(bool visible) override {
      remember_geometry.setVisible_override<superclass>(this, visible);
    }

    void hideEvent(QHideEvent *event_) override {
      remember_geometry.hideEvent_override(this);
    }

   */

  struct RememberGeometry{
    QByteArray geometry;
    bool has_stored_geometry = false;

    void save(QWidget *widget){
      geometry = widget->saveGeometry();
      has_stored_geometry = true;
      //printf("   SAVING geometry\n");
    }

    void restore(QWidget *widget){
      if (has_stored_geometry){

        //printf("   RESTORING geometry\n");
        widget->restoreGeometry(geometry);
        
      }else{

        //printf("   88888888888888888888888888888888888888888888 NO geometry stored\n");

        moveWindowToCentre(widget);
      }
    }

    ///////////////////
    // I've tried lots of things, and the only thing that seems to work is overriding setVisible and hideEvent exactly like below.
    // Other combinations will fail in more or less subtle ways.
    ///////////////////
  
    template <class SuperWidget>
    void setVisible_override(SuperWidget *widget, bool visible) {
      if (visible==false && widget->isVisible()==false){
        widget->SuperWidget::setVisible(visible);
        return;
      }
      if (visible==true && widget->isVisible()==true){
        widget->SuperWidget::setVisible(visible);
        return;
      }

      if (visible && widget->window()==widget)
        restore(widget);  // Not necessary for correct operation, but it might seem like it removes some flickering.
     
      widget->SuperWidget::setVisible(visible);
      
      if (visible && widget->window()==widget)
        restore(widget);
    }

    void hideEvent_override(QWidget *widget) {
      if (widget->window()==widget)
        save(widget);
    }

  };


struct RememberGeometryQDialog : public QDialog {

  RememberGeometry remember_geometry;

#if PUT_ON_TOP
  
  static int num_open_dialogs;

  struct Timer : public QTimer {
    bool was_visible = false;
    
    QWidget *parent;
    Timer(QWidget *parent)
      :parent(parent)
    {
      setInterval(200);
      start();      
    }

    ~Timer(){
      if (was_visible)
        RememberGeometryQDialog::num_open_dialogs--;
    }
      

    void timerEvent ( QTimerEvent * e ){
      //printf("Raising parent\n");
      bool is_visible = parent->isVisible();
      
      if(is_visible && !was_visible) {
        was_visible = true;
        RememberGeometryQDialog::num_open_dialogs++;
      }
          
      if(!is_visible && was_visible) {
        was_visible = false;
        RememberGeometryQDialog::num_open_dialogs--;
      }
          
      if (is_visible && RememberGeometryQDialog::num_open_dialogs==1)
        parent->raise(); // This is probably a bad idea.
    }
  };
  

  Timer timer;
#endif
  
public:
   RememberGeometryQDialog(QWidget *parent_, bool is_modal)
    : QDialog(parent_!=NULL ? parent_ : g_main_window, Qt::Window) // | Qt::Tool)
#if PUT_ON_TOP
    , timer(this)
#endif
  {
    set_window_flags(this, is_modal);
    //QDialog::setWindowModality(Qt::ApplicationModal);
    //setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
  }
  
  virtual void setVisible(bool visible) override {
    remember_geometry.setVisible_override<QDialog>(this, visible);
  }
  
  // See comment in helpers.h for the radium::RememberGeometry class.
  virtual void hideEvent(QHideEvent *event_) override {
    remember_geometry.hideEvent_override(this);
  }

};

}

struct GL_PauseCaller{
  GL_PauseCaller(){
    bool locked = GL_maybeLock();
    GL_pause_gl_thread_a_short_while();
    if (locked)
      GL_unlock();      
  }
};


namespace radium{
struct ScopedExec{
  bool _lock;
  
  ScopedExec(bool lock=true)
    : _lock(lock)
  {      
    obtain_keyboard_focus();
    g_radium_runs_custom_exec = true;
    
    GFX_HideProgress();
    
    if (_lock)  // GL_lock <strike>is</strike> was needed when using intel gfx driver to avoid crash caused by opening two opengl contexts simultaneously from two threads.
      GL_lock();
  }
  
  ~ScopedExec(){
    if (_lock)
      GL_unlock();
    GFX_ShowProgress();
    g_radium_runs_custom_exec = false;

    for(auto *window : QApplication::topLevelWidgets())
      updateWidgetRecursively(window);

    release_keyboard_focus();
  }
};
}


// Happens sometimes that there are more than two menues visible at the same time. (probably got something to do with non-async menues)
static inline void closePopup(void){
  if (!g_curr_popup_qmenu.isNull()){
    g_curr_popup_qmenu->hide(); // safer.
  //g_curr_popup_qmenu->deleteLater(); // We might be called from the "triggered" callback of the menu.
  }
}

static inline void set_those_menu_variables_when_starting_a_popup_menu(QMenu *menu_to_be_started){
  if (!g_curr_popup_qmenu.isNull()) // Don't set it if there's a menu open already.
    g_current_parent_before_qmenu_opened = get_current_parent(false, false);

  g_curr_popup_qmenu = menu_to_be_started;
}

static inline int safeExec(QMessageBox *widget){
  R_ASSERT_RETURN_IF_FALSE2(g_qt_is_painting==false, -1);
    
  closePopup();

#if defined(RELEASE)
  // QMessageBox is usually used to show error and warning messages.
  R_ASSERT(g_radium_runs_custom_exec==false);
#endif

  if (g_radium_runs_custom_exec==true){
    GFX_HideProgress();
    GL_lock();
    int ret = widget->exec();
    GFX_ShowProgress();
    GL_unlock();
    return ret;
  }

  radium::ScopedExec scopedExec;

  return widget->exec();
}

/*
static inline int safeExec(QMessageBox &widget){
  closePopup();

  R_ASSERT_RETURN_IF_FALSE2(g_radium_runs_custom_exec==false, 0);

  radium::ScopedExec scopedExec;

  return widget.exec();
}
*/

static inline int safeExec(QDialog *widget){
  R_ASSERT_RETURN_IF_FALSE2(g_qt_is_painting==false, -1);
  
  closePopup();

  R_ASSERT_RETURN_IF_FALSE2(g_radium_runs_custom_exec==false, 0);

  radium::ScopedExec scopedExec;

  return widget->exec();
}

static inline QAction *safeMenuExec(QMenu *widget){
  R_ASSERT(g_qt_is_painting==false);
  
  // safeExec might be called recursively if you double click right mouse button to open a pop up menu. Seems like a bug or design flaw in Qt.
#if !defined(RELEASE)
  if (g_radium_runs_custom_exec==true)
    GFX_Message(NULL, "Already runs custom exec");
#endif

  closePopup();
  set_those_menu_variables_when_starting_a_popup_menu(widget);
      
  if (doModalWindows()) {
    radium::ScopedExec scopedExec;
    return widget->exec(QCursor::pos());
  }else{
    radium::ScopedExec scopedExec(false);
    GL_lock();{
      GL_pause_gl_thread_a_short_while();
    }GL_unlock();    
    return widget->exec(QCursor::pos());
  }
}

static inline void safeMenuPopup(QMenu *menu){
  R_ASSERT_RETURN_IF_FALSE(g_qt_is_painting==false);
  
  closePopup();
  set_those_menu_variables_when_starting_a_popup_menu(menu);

  menu->popup(QCursor::pos());
}

static inline void safeShow(QWidget *widget){
  R_ASSERT_RETURN_IF_FALSE(g_qt_is_painting==false);
  
  closePopup();

  GL_lock(); {
    GL_pause_gl_thread_a_short_while();
    widget->show();
    widget->raise();
    widget->activateWindow();
#if 0 //def FOR_WINDOWS
    HWND wnd=(HWND)w->winId();
    SetFocus(wnd);
    SetWindowPos(wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
#endif
  }GL_unlock();
}

static inline void safeShowOrExec(QDialog *widget){
  if (doModalWindows())
    safeExec(widget);
  else
    safeShow(widget);
}

#endif // RADIUM_QT_HELPERS
