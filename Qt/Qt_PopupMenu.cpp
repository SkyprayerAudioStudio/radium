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

#if USE_QT_MENU

#include <QStack>
#include <QAction>
#include <QWidgetAction>
#include <QCheckBox>
#include <QMenu>

#include "../common/nsmtracker.h"
#include "../common/visual_proc.h"
#include "../embedded_scheme/s7extra_proc.h"

#include "../api/api_proc.h"

#include "EditorWidget.h"




namespace{

  static int MyQMenu_g_num = -1;  // Workaround This is really strange. I can not make QMenu::exec() return the action. It only returns NULL. (Must be static, since qmenu is deleted after exec() has returned).


  struct MyQMenu : public QMenu{

    func_t *_callback;

    MyQMenu(QWidget *parent, bool is_async, func_t *callback)
      : QMenu(parent)
      , _callback(callback)
    {

      if (!is_async)      
        MyQMenu_g_num = -1;

      if(_callback!=NULL)
        s7extra_protect(_callback);
    }
    
    ~MyQMenu(){
      if(_callback!=NULL)
        s7extra_unprotect(_callback);      
    }

    bool _has_keyboard_focus = false;

    void showEvent(QShowEvent *event) override {
      if (_has_keyboard_focus==false){
        obtain_keyboard_focus_counting();
        _has_keyboard_focus = true;
      }
    }

    void hideEvent(QHideEvent *event) override {
      if (_has_keyboard_focus==true){
        release_keyboard_focus_counting();
        _has_keyboard_focus = false;
      }
    }

    void closeEvent(QCloseEvent *event) override{
      if (_has_keyboard_focus==true){
        release_keyboard_focus_counting();
        _has_keyboard_focus = false;
      }
    }
  };

  class CheckableAction : public QWidgetAction
  {
    Q_OBJECT

    QString text;
    MyQMenu *qmenu;
    int num;
    bool is_async;
    func_t *callback;
    std::function<void(int,bool)> callback3;
    
  public:

    ~CheckableAction(){
      //printf("I was deleted: %s\n",text.toUtf8().constData());
    }
    
    CheckableAction(const QString & text_b, bool is_on, MyQMenu *qmenu_b, int num_b, bool is_async, func_t *callback_b, std::function<void(int,bool)> callback3_b)
      : QWidgetAction(qmenu_b)
      , text(text_b)
      , qmenu(qmenu_b)
      , num(num_b)
      , is_async(is_async)
      , callback(callback_b)
      , callback3(callback3_b)
    {
      QWidget *widget = new QWidget;
      QHBoxLayout *layout = new QHBoxLayout;
      layout->setSpacing(0);
      layout->setContentsMargins(0,0,0,0);

      layout->addItem(new QSpacerItem(gui_textWidth("F"), 10, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));
      
      QCheckBox *checkBox = new QCheckBox(text, qmenu);
      checkBox->setChecked(is_on);

      layout->addWidget(checkBox);
            
      widget->setLayout(layout);
      setDefaultWidget(widget);

      connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(toggled(bool)));      
    }

  public slots:
    void toggled(bool checked){
    //void clicked(bool checked){
      printf("CLICKED %d\n",checked);
      if (callback!=NULL)
        callFunc_void_int_bool(callback, num, checked);
      if (callback3)
        callback3(num, checked);

      if (!is_async)
        MyQMenu_g_num = num; // workaround

      //if (is_async)
        qmenu->close();
      //delete parent;
    }
  };

  class ClickableAction : public QAction
  {
    Q_OBJECT

    QString text;
    MyQMenu *qmenu;
    int num;
    bool is_async;
    func_t *callback;
    std::function<void(int,bool)> callback3;

  public:

    ~ClickableAction(){
      //printf("I was deleted: %s\n",text.toUtf8().constData());
    }
    
    ClickableAction(const QString & text, MyQMenu *qmenu, int num, bool is_async, func_t *callback, std::function<void(int,bool)> callback3)
      : QAction(text, qmenu)
      , text(text)
      , qmenu(qmenu)
      , num(num)
      , is_async(is_async)
      , callback(callback)
      , callback3(callback3)
    {
      connect(this, SIGNAL(triggered()), this, SLOT(triggered()));      
    }

  public slots:
    void triggered(){
    //void clicked(bool checked){
      //fprintf(stderr,"CLICKED clickable\n");

      if (callback!=NULL)
        s7extra_callFunc_void_int(callback, num);

      fprintf(stderr,"CLICKED 222222 clickable %d\n", num);
            
      if (callback3)
        callback3(num, true);

      if (!is_async)
        MyQMenu_g_num = num; // workaround

      //if (is_async)
        qmenu->close();

      //delete parent;
    }
  };
}

QPointer<QWidget> g_current_parent_before_qmenu_opened; // Only valid if g_curr_popup_qmenu != NULL
QPointer<QMenu> g_curr_popup_qmenu;



static QMenu *create_qmenu(
                           const vector_t &v,
                           bool is_async,
                           func_t *callback2,
                           std::function<void(int,bool)> callback3
                           )
{
  MyQMenu *menu = new MyQMenu(NULL, is_async, callback2);
  menu->setAttribute(Qt::WA_DeleteOnClose);
  
  QMenu *curr_menu = menu;
  
  QStack<QMenu*> parents;
  QStack<int> n_submenuess;
  int n_submenues=0;
  
  for(int i=0;i<v.num_elements;i++) {
    QString text = (const char*)v.elements[i];
    if (text.startsWith("----"))
      menu->addSeparator();
    else {
      
      if (n_submenues==getMaxSubmenuEntries()){
        curr_menu = curr_menu->addMenu("Next");
        //curr_menu->setStyleSheet("QMenu { menu-scrollable: 1; }");
        n_submenues=0;
      }
      
      QAction *action = NULL;
      
      
      bool disabled = false;
      
      if (text.startsWith("[disabled]")){
        text = text.right(text.size() - 10);
        disabled = true;
      }
      
      if (text.startsWith("[check ")){
        
        if (text.startsWith("[check on]"))
          action = new CheckableAction(text.right(text.size() - 10), true, menu, i, is_async, callback2, callback3);
        else
          action = new CheckableAction(text.right(text.size() - 11), false, menu, i, is_async, callback2, callback3);
        
      } else if (text.startsWith("[submenu start]")){
        
        n_submenuess.push(n_submenues);
        n_submenues = 0;
        parents.push(curr_menu);
        curr_menu = curr_menu->addMenu(text.right(text.size() - 15));
        //curr_menu->setStyleSheet("QMenu { menu-scrollable: 1; }");
        
      } else if (text.startsWith("[submenu end]")){
        
        QMenu *parent = parents.pop();
        if (parent==NULL)
          RError("parent of [submenu end] is not a QMenu");
        else
          curr_menu = parent;
        n_submenues = n_submenuess.pop();
        
      } else {
        
        action = new ClickableAction(text, menu, i, is_async, callback2, callback3);
        
      }
      
      if (action != NULL){
        action->setData(i);
        curr_menu->addAction(action);  // are these actions automatically freed in ~QMenu? (yes, seems so) (they are probably freed because they are children of the qmenu)
      }
      
      if (disabled)
        action->setDisabled(true);
      
      n_submenues++;
    }
  }
  
  return menu;
}


static int GFX_QtMenu(
                const vector_t &v,
                func_t *callback2,
                std::function<void(int,bool)> callback3,
                bool is_async
                )
{

  if(is_async)
    R_ASSERT(callback2!=NULL || callback3);

  QMenu *menu = create_qmenu(v, is_async,  callback2, callback3);
  //printf("                CREATED menu %p", menu);
  
  if (is_async){
    
    safeMenuPopup(menu);
    return -1;
    
  } else {
    
    QAction *action = safeMenuExec(menu);
    (void)action;
    //printf("    ACTION: %p\n", action);

    return MyQMenu_g_num; // Strange stuff.
    /*
    if(action==NULL) {
      
      return -1;
    
    if (dynamic_cast<CheckableAction*>(action) != NULL)
      return -1;
    
    bool ok;
    int i=action->data().toInt(&ok);
    
    if (ok)      
      return i;

    //RWarning("Got unknown action %p %s\n",action,action->text().toAscii().constData());
    
    return -1;
    */
  }
}
void GFX_Menu3(
              const vector_t &v,
              std::function<void(int,bool)> callback3
              )
{
  GFX_QtMenu(v, NULL, callback3, true);
}

int GFX_Menu2(
              struct Tracker_Windows *tvisual,
              ReqType reqtype,
              const char *seltext,
              const vector_t v,
              func_t *callback,
              bool is_async
              )
{
  if(is_async)
    R_ASSERT(callback!=NULL);

  if(reqtype==NULL || v.num_elements>20 || is_async || callback!=NULL){
    std::function<void(int,bool)> empty_callback3;
    return GFX_QtMenu(v, callback, empty_callback3, is_async);
  }else
    return GFX_ReqTypeMenu(tvisual,reqtype,seltext,v);
}


int GFX_Menu(
             struct Tracker_Windows *tvisual,
             ReqType reqtype,
             const char *seltext,
             const vector_t v
             )
{
  return GFX_Menu2(tvisual, reqtype, seltext, v, NULL, false);
}

// The returned vector can be used as argument for GFX_Menu.
vector_t GFX_MenuParser(const char *texts, const char *separator){
  vector_t ret = {};
  
  QStringList splitted = QString(texts).split(separator);
  
  for(int i=0 ; i<splitted.size() ; i++){
    QString trimmed = splitted[i].trimmed();
    if (trimmed != "")
      VECTOR_push_back(&ret, talloc_strdup(trimmed.toUtf8().constData()));
  }

  return ret;
}

#include "mQt_PopupMenu.cpp"

#endif // USE_QT_MENU
