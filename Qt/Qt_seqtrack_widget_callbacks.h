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

#include <math.h>

#include <QColor>
#include <QRawFont>
#include <QApplication>

#include "Qt_MyQCheckBox.h"
#include "Qt_MyQSlider.h"

#include "Qt_seqtrack_widget.h"

#include "../embedded_scheme/scheme_proc.h"
#include "../common/undo.h"
#include "../common/song_tempo_automation_proc.h"
#include "../common/tracks_proc.h"


#include "../common/seqtrack_proc.h"

static bool g_need_update = false;

static bool smooth_scrolling(void){
  return smoothSequencerScrollingEnabled();
}

static bool g_draw_colored_seqblock_tracks = true;

namespace{
  struct GlyphpathAndWidth{
    QPainterPath path;
    float width;
    QFontMetricsF fn;
    GlyphpathAndWidth(QPainterPath _path, float _width, const QFontMetricsF &_fn)
      : path(_path)
      , width(_width)
      , fn(_fn)
    {}
    GlyphpathAndWidth()
      : width(0.0f)
      , fn(QFontMetricsF(QApplication::font()))
    {}
  };
}

static GlyphpathAndWidth getGlyphpathAndWidth(const QFont &font, const QChar c){
  static QFont cacheFont = font;
  static QRawFont rawFont = QRawFont::fromFont(font);
  static QHash<QChar,GlyphpathAndWidth> glyphpathCache;
  static QFontMetricsF fn(font);

  if (font != cacheFont){
    glyphpathCache.clear();
    cacheFont = font;
    rawFont = QRawFont::fromFont(font);
    fn = QFontMetrics(font);
  }

  if (glyphpathCache.contains(c))
    return glyphpathCache[c];


  QVector<quint32> indexes = rawFont.glyphIndexesForString(c);

  GlyphpathAndWidth g(rawFont.pathForGlyph(indexes[0]), fn.width(c), fn);
  
  glyphpathCache[c] = g;

  return g;
}

// QPainter::drawText doesn't have floating point precision.
//
static void myDrawText(QPainter &painter, QRectF rect, QString text, QTextOption option = QTextOption(Qt::AlignLeft | Qt::AlignTop)){
  if (is_playing() && pc->playtype==PLAYSONG && smooth_scrolling() && smooth_scrolling()){
    int len=text.length();

    float x = rect.x();
    float y = -1;
    
    QBrush brush = painter.pen().brush();
    
    for(int i=0; i<len; i++){
      GlyphpathAndWidth g = getGlyphpathAndWidth(painter.font(), text.at(i));
      if (y < 0)
        y = rect.y()+g.fn.ascent();//+g.fn.descent();//height();//2+rect.height()/2;      
      painter.save();
      painter.translate(x, y);
      painter.fillPath(g.path,brush);
      painter.restore();
      
      x += g.width;
    }
  } else {
    painter.drawText(rect, text, option);
  } 
}

static void myFillRect(QPainter &p, QRectF rect, const QColor &color){
  QPen pen = p.pen();
  p.setPen(Qt::NoPen);
  p.setBrush(color);
  p.drawRect(rect);
  p.setBrush(Qt::NoBrush);
  p.setPen(pen);
}

static void g_position_widgets(void);

static QPoint mapFromEditor(QWidget *widget, QPoint point){
  QPoint global = g_editor->mapToGlobal(point);
  return widget->mapFromGlobal(global);
}

static QPoint mapToEditor(QWidget *widget, QPoint point){
  //return widget->mapTo(g_editor, point); (g_editor must be a parent, for some reason)
  QPoint global = widget->mapToGlobal(point);
  return g_editor->mapFromGlobal(global);
}

static float mapToEditorX(QWidget *widget, float x){
  //auto global = widget->mapToGlobal(QPoint(x, 0));
  //return g_editor->mapFromGlobal(global).x();
  return mapToEditor(widget, QPoint(x, 0)).x();
}

static float mapToEditorY(QWidget *widget, float y){
  //auto global = widget->mapToGlobal(QPoint(0, y));
  //return g_editor->mapFromGlobal(global).y();
  return mapToEditor(widget, QPoint(0, y)).y();
}

static float mapToEditorX1(QWidget *widget){
  //auto global = widget->mapToGlobal(QPoint(x, 0));
  //return g_editor->mapFromGlobal(global).x();
  return mapToEditor(widget, QPoint(0, 0)).x();
}

static float mapToEditorY1(QWidget *widget){
  //auto global = widget->mapToGlobal(QPoint(0, y));
  //return g_editor->mapFromGlobal(global).y();
  return mapToEditor(widget, QPoint(0, 0)).y();
}

static float mapToEditorX2(QWidget *widget){
  //auto global = widget->mapToGlobal(QPoint(x, 0));
  //return g_editor->mapFromGlobal(global).x();
  return mapToEditor(widget, QPoint(0, 0)).x() + widget->width();
}

static float mapToEditorY2(QWidget *widget){
  //auto global = widget->mapToGlobal(QPoint(0, y));
  //return g_editor->mapFromGlobal(global).y();
  return mapToEditor(widget, QPoint(0, 0)).y() + widget->height();
}

/*
static double getBlockAbsDuration(const struct Blocks *block){
  return getBlockSTimeLength(block) * ATOMIC_DOUBLE_GET(block->reltempo);
}
*/

static QColor get_block_color(const struct Blocks *block){
  //return mix_colors(QColor(block->color), get_qcolor(SEQUENCER_BLOCK_BACKGROUND_COLOR_NUM), 0.32f);
  return QColor(block->color);
}

class MouseTrackerQWidget : public QWidget {
public:
  
  MouseTrackerQWidget(QWidget *parent)
    : QWidget(parent)
  {
    setMouseTracking(true);
  }
  
  int _currentButton = 0;

  int getMouseButtonEventID( QMouseEvent *qmouseevent){
    if(qmouseevent->button()==Qt::LeftButton)
      return TR_LEFTMOUSEDOWN;
    else if(qmouseevent->button()==Qt::RightButton)
      return TR_RIGHTMOUSEDOWN;
    else if(qmouseevent->button()==Qt::MiddleButton)
      return TR_MIDDLEMOUSEDOWN;
    else
      return 0;
  }

  void	mousePressEvent( QMouseEvent *event) override{
    event->accept();
    _currentButton = getMouseButtonEventID(event);
    QPoint point = mapToEditor(this, event->pos());
    SCHEME_mousepress(_currentButton, point.x(), point.y());
    //printf("  Press. x: %d, y: %d. This: %p\n", point.x(), point.y(), this);
  }
  void	mouseReleaseEvent( QMouseEvent *event) override{
    event->accept();
    QPoint point = mapToEditor(this, event->pos());
    SCHEME_mouserelease(_currentButton, point.x(), point.y());
    _currentButton = 0;
    //printf("  Release. x: %d, y: %d. This: %p\n", point.x(), point.y(), this);
  }
  void	mouseMoveEvent( QMouseEvent *event) override{
    event->accept();
    QPoint point = mapToEditor(this, event->pos());
    SCHEME_mousemove(_currentButton, point.x(), point.y());
    //printf("    move. x: %d, y: %d. This: %p\n", point.x(), point.y(), this);
  }

};


static void handle_wheel_event(QWheelEvent *e, int x1, int x2, double start_play_time, double end_play_time) {
      
  if (  (e->modifiers() & Qt::ControlModifier) || (e->modifiers() & Qt::ShiftModifier)) {

    double nav_start_time = SEQUENCER_get_visible_start_time();
    double nav_end_time = SEQUENCER_get_visible_end_time();
    double visible_song_length = SONG_get_gfx_length() * MIXER_get_sample_rate();
    double new_start,new_end;
    
    double range = nav_end_time - nav_start_time;
    double middle = nav_start_time + range/2.0;
    double how_much = range / 10.0;
     
    if (e->modifiers() & Qt::ControlModifier) {
      
      if (e->delta() > 0) {
        
        new_start = nav_start_time + how_much;
        new_end = nav_end_time - how_much;
        
      } else {
        
        new_start = nav_start_time - how_much;
        new_end = nav_end_time + how_much;
        
      }
      
      if (fabs(new_end-new_start) < 400 || new_end<=new_start) {
        new_start = middle - 200;
        new_end = middle + 200;
      }
      
    } else {

      if (e->delta() > 0) {
        
        new_start = nav_start_time + how_much;
        new_end = nav_end_time + how_much;
        
      } else {
        
        new_start = nav_start_time - how_much;
        new_end = nav_end_time - how_much;
        
      }
      
    }

    if (new_start < 0){
      new_end -= new_start;
      new_start = 0;
    }
    
    if (new_end > visible_song_length){
      double over = new_end - visible_song_length;
      new_end = visible_song_length;
      new_start -= over;
    }
      
    SEQUENCER_set_visible_start_and_end_time(new_start, new_end);

  } else {

    int64_t pos = R_MAX(0, scale(e->x(), x1, x2, start_play_time, end_play_time));
    if (e->delta() > 0)
      PlaySong(pos);
    else {
      PlayStop();
      ATOMIC_DOUBLE_SET(pc->song_abstime, pos);
      SEQUENCER_update();
    }
  }
}


class Seqblocks_widget {
public:

  //QVector<Seqblock_widget*> _seqblock_widgets; // deleted automatically when 'this' is deleted.
  
  QWidget *_sequencer_widget;
  
  SeqTrack *_seqtrack;
  
  const double &_start_time;
  const double &_end_time;
  QTime _time;
  QRectF _rect;

  float t_x1,t_y1,t_x2,t_y2,width,height;
  
  Seqblocks_widget(QWidget *_sequencer_widget, SeqTrack *seqtrack, const double &start_time, const double &end_time)
    : _sequencer_widget(_sequencer_widget)
    , _seqtrack(add_gc_root(seqtrack))
    , _start_time(start_time)
    , _end_time(end_time)
  {
    position_widgets(0,0,100,100);
    
    //create_seqblock_widgets();
    _time.start();
  }
  
  ~Seqblocks_widget(){
    remove_gc_root(_seqtrack);

    printf("Seqblocks widget deleted\n");
    //getchar();
  }

  void position_widgets(float x1, float y1, float x2, float y2){
    t_x1 = x1;
    t_y1 = y1;
    t_x2 = x2;
    t_y2 = y2;
    height = t_y2-t_y1;
    width = t_x2-t_x1;
    
    _rect = QRectF(t_x1, t_y1, width, height);
  }
  
  /*
  void wheelEvent(QWheelEvent *e) override {
    handle_wheel_event(e, 0, width(), _start_time, _end_time);
  }
  */
  
  float get_seqblock_x1(struct SeqBlock *seqblock, double start_time, double end_time) const {
    return scale(seqblock->start_time, start_time, end_time, t_x1, t_x2);
  }
  
  float get_seqblock_x2(struct SeqBlock *seqblock, double start_time, double end_time) const {
    return scale(seqblock->end_time, start_time, end_time, t_x1, t_x2);
  }

  float get_seqblock_x1(int seqblocknum) const {
    R_ASSERT_RETURN_IF_FALSE2(seqblocknum>=0, 0);

    // This can happen while the sequencer is updated.
    if (seqblocknum >= _seqtrack->seqblocks.num_elements)
      return 0;
    
    SEQTRACK_update_all_seqblock_gfx_start_and_end_times(_seqtrack);
    double start_time = _start_time / MIXER_get_sample_rate();
    double end_time = _end_time / MIXER_get_sample_rate();
    return get_seqblock_x1((struct SeqBlock*)_seqtrack->seqblocks.elements[seqblocknum], start_time, end_time);
  }

  float get_seqblock_x2(int seqblocknum) const {
    R_ASSERT_RETURN_IF_FALSE2(seqblocknum>=0, 0);

    // This can happen while the sequencer is updated.
    if (seqblocknum >= _seqtrack->seqblocks.num_elements)
      return 100;

    SEQTRACK_update_all_seqblock_gfx_start_and_end_times(_seqtrack);
    double start_time = _start_time / MIXER_get_sample_rate();
    double end_time = _end_time / MIXER_get_sample_rate();
    return get_seqblock_x2((struct SeqBlock*)_seqtrack->seqblocks.elements[seqblocknum], start_time, end_time);
  }

  void set_seqblocks_is_selected(const QRect &selection_rect){
    double sample_rate = MIXER_get_sample_rate();
    //double song_length = get_visible_song_length()*sample_rate;
  
    SEQTRACK_update_all_seqblock_gfx_start_and_end_times(_seqtrack);

    double start_time = _start_time / sample_rate;
    double end_time = _end_time / sample_rate;

    VECTOR_FOR_EACH(struct SeqBlock *, seqblock, &_seqtrack->seqblocks){

      if (seqblock->start_time < end_time && seqblock->end_time >= start_time) {
        float x1 = get_seqblock_x1(seqblock, start_time, end_time);
        float x2 = get_seqblock_x2(seqblock, start_time, end_time);
        
        QRect rect(x1,t_y1+1,x2-x1,height-2);
        
        seqblock->is_selected = rect.intersects(selection_rect);
      } else {
        seqblock->is_selected = false;
      }

    }END_VECTOR_FOR_EACH;
     
  }

  void paintTrack(QPainter &p, float x1, float y1, float x2, float y2, const struct Blocks *block, const struct Tracks *track, int64_t blocklen, bool is_multiselected) const {
    QColor color1 = get_qcolor(SEQUENCER_NOTE_COLOR_NUM);
    QColor color2 = get_qcolor(SEQUENCER_NOTE_START_COLOR_NUM);
    
#define SHOW_BARS 0

#if SHOW_BARS
    p.setBrush(QBrush(color));
#else
    const float bar_height = 2.3;
    const float bar_header_length = 3.2;
    
    QPen pen1(color1);
    pen1.setWidthF(bar_height);
    pen1.setCapStyle(Qt::FlatCap);

    QPen pen2(pen1);
    pen2.setColor(color2);

    {
      QColor color;

      if (is_multiselected)
        color = get_block_qcolor(SEQUENCER_BLOCK_MULTISELECT_BACKGROUND_COLOR_NUM, true);

      else if (!g_draw_colored_seqblock_tracks)
        color = get_block_qcolor(SEQUENCER_BLOCK_BACKGROUND_COLOR_NUM, false);
      
      else if (track->patch!=NULL)
        color = QColor(track->patch->color);

      else
        goto no_track_background;
      
      QRectF rect(x1,y1,x2-x1,y2-y1);

      myFillRect(p, rect, color);
    }
    
  no_track_background:
    
#endif

    float track_pitch_min;
    float track_pitch_max;
    TRACK_get_min_and_max_pitches(track, &track_pitch_min, &track_pitch_max);
    
    struct Notes *note = track->notes;
    while(note != NULL){

      int64_t start = Place2STime(block, &note->l.p);
      int64_t end = Place2STime(block, &note->end);

      float n_x1 = scale(start, 0, blocklen, x1, x2);
      float n_x2 = scale(end, 0, blocklen, x1, x2);

      p.setPen(pen1);

#if SHOW_BARS
      float n_y1 = scale(note->note, 127, 0, y1, y2);
      float n_y2 = y2;
      
      QRectF rect(n_x1, n_y1, n_x2-n_x1, n_y2-n_y1);
      p.drawRect(rect);
      
#else
      float n_y = track_pitch_max==track_pitch_min ? (y1+y2)/2.0 : scale(note->note+0.5, track_pitch_max, track_pitch_min, y1, y2);

      {
        float x2 = R_MIN(n_x2, n_x1+bar_header_length);
        
        QLineF line(n_x1,n_y,x2,n_y);
        p.setPen(pen2);
        p.drawLine(line);

        if (n_x2 > x2){
          QLineF line(x2,n_y,n_x2,n_y);
          p.setPen(pen1);
          p.drawLine(line);
        }
      }
#endif
#undef SHOW_BARS
      
      note = NextNote(note);
    }
  }
  
  QColor half_alpha(QColor c, bool is_gfx) const {
    if (is_gfx)
      c.setAlpha(c.alpha() / 4);      
    return c;
  }

  QColor get_block_qcolor(enum ColorNums colornum, bool is_gfx) const {
    QColor c = get_qcolor(colornum);
    return half_alpha(c, is_gfx);
  }

  void paintBlock(QPainter &p, const QRectF &rect, const struct SeqBlock *seqblock, bool is_gfx){
    const struct Blocks *block = seqblock->block;

    const int header_height = root->song->tracker_windows->systemfontheight*1.3;
    
    QColor text_color = get_block_qcolor(SEQUENCER_TEXT_COLOR_NUM, is_gfx);
    QColor border_color = get_block_qcolor(SEQUENCER_BLOCK_BORDER_COLOR_NUM, is_gfx);

    QColor header_border_color = get_block_qcolor(SEQUENCER_TRACK_BORDER1_COLOR_NUM, is_gfx);
    QColor track_border_color  = get_block_qcolor(SEQUENCER_TRACK_BORDER2_COLOR_NUM, is_gfx);

    QPen header_border_pen(header_border_color);
    QPen track_border_pen(track_border_color);

    header_border_pen.setWidthF(2.3);
    track_border_pen.setWidthF(1.3);

    bool is_current_block = block == root->song->tracker_windows->wblock->block;

    qreal x1,y1,x2,y2;
    rect.getCoords(&x1, &y1, &x2, &y2);

    if (true || is_current_block) {
      QRectF rect1(x1, y1, x2-x1, header_height);
      //QRectF rect2(x1, y1+header_height, x2-x1, y2-(y1+header_height));

      myFillRect(p, rect1, half_alpha(get_block_color(block), is_gfx));
      //myFillRect(p, rect2, get_block_qcolor(is_gfx ? SEQUENCER_BLOCK_MULTISELECT_BACKGROUND_COLOR_NUM : SEQUENCER_BLOCK_BACKGROUND_COLOR_NUM, is_gfx));
      
    } else {
      //myFillRect(p, rect, get_block_qcolor(SEQUENCER_BLOCK_BACKGROUND_COLOR_NUM, is_gfx));
    }

    //if (x1 > -5000) { // avoid integer overflow error.
    p.setPen(text_color);
    //p.drawText(x1+4,2,x2-x1-6,height()-4, Qt::AlignLeft, QString::number(seqblock->block->l.num) + ": " + seqblock->block->name);
    myDrawText(p, rect.adjusted(2,1,-4,-(rect.height()-header_height)), QString::number(block->l.num) + ": " + block->name);
    //}

    if (is_current_block){
      QColor c = get_block_qcolor(CURSOR_EDIT_ON_COLOR_NUM, is_gfx);
      c.setAlpha(150);      
      p.setPen(QPen(c, 4));
    } else {
      p.setPen(border_color);
    }

    p.drawRoundedRect(rect,1,1);

    int64_t blocklen = getBlockSTimeLength(block);

    // Tracks
    {
      int num_tracks = block->num_tracks;
      
      struct Tracks *track = block->tracks;
      while(track != NULL){
        
        float t_y1 = scale(track->l.num,0,num_tracks,y1+header_height,y2);
        float t_y2 = scale(track->l.num+1,0,num_tracks,y1+header_height,y2);
        
        // Draw track border
        {
          p.setPen(track->l.num==0 ? header_border_pen : track_border_pen);        
          p.drawLine(QLineF(x1,t_y1,x2,t_y1));
        }
        
        // Draw track
        paintTrack(p, x1, t_y1, x2, t_y2, block, track, blocklen, is_gfx);
        
        track = NextTrack(track);
      }
    }

    // Bar and beats
    if(1){
      struct Beats *beat = block->beats;
      QColor bar_color = get_block_qcolor(SEQUENCER_BLOCK_BAR_COLOR_NUM, is_gfx);
      QColor beat_color = get_block_qcolor(SEQUENCER_BLOCK_BEAT_COLOR_NUM, is_gfx);
      QPen bar_pen(bar_color);
      QPen beat_pen(beat_color);

      bar_pen.setWidth(1.3);
      beat_pen.setWidth(1.3);

      if (beat!=NULL)
        beat = NextBeat(beat);
      
      while(beat != NULL){
        
        float b_y1 = y1+header_height;
        float b_y2 = y2;

        float pos = Place2STime(block, &beat->l.p);
        
        float b_x = scale(pos, 0, blocklen, x1, x2);

        if (beat->beat_num==1)
          p.setPen(bar_pen);
        else
          p.setPen(beat_pen);

        QLineF line(b_x, b_y1, b_x, b_y2);
        p.drawLine(line);
        
        beat = NextBeat(beat);
      }
    }

    //printf("Seqblock: %p, %d\n", seqblock, seqblock->is_selected);
    if (seqblock->is_selected){
      QColor grayout_color = get_block_qcolor(SEQUENCER_BLOCK_SELECTED_COLOR_NUM, is_gfx);

      p.setPen(Qt::NoPen);
      p.setBrush(grayout_color);
      
      p.drawRect(rect);

      p.setBrush(Qt::NoBrush);
    }
  }
  
  void paint(const QRect &update_rect, QPainter &p){ // QPaintEvent * ev ) override {

    //printf("  PAINTING %d %d -> %d %d\n",t_x1,t_y1,t_x2,t_y2);

    myFillRect(p, _rect.adjusted(1,1,-2,-1), get_qcolor(SEQUENCER_BACKGROUND_COLOR_NUM));
      
    double sample_rate = MIXER_get_sample_rate();
    //double song_length = get_visible_song_length()*sample_rate;
  
    SEQTRACK_update_all_seqblock_gfx_start_and_end_times(_seqtrack);

    double start_time = _start_time / sample_rate;
    double end_time = _end_time / sample_rate;

    for(int i=0;i<2;i++){
      vector_t *seqblocks = i==0 ? &_seqtrack->seqblocks : &_seqtrack->gfx_gfx_seqblocks ;

      VECTOR_FOR_EACH(struct SeqBlock *, seqblock, seqblocks){
        
        //printf("      PAINTING BLOCK %f / %f, seqtrack/seqblock: %p / %p\n", seqblock->start_time, seqblock->end_time, _seqtrack, seqblock);

        if (seqblock->start_time < end_time && seqblock->end_time >= start_time) {
          
          float x1 = get_seqblock_x1(seqblock, start_time, end_time);
          float x2 = get_seqblock_x2(seqblock, start_time, end_time);
          //if (i==1)
          //  printf("   %d: %f, %f. %f %f\n", iterator666, x1, x2, seqblock->start_time / 44100.0, seqblock->end_time / 44100.0);

          QRectF rect(x1,t_y1+1,x2-x1,height-2);
          
          if (update_rect.intersects(rect.toAlignedRect()))
            paintBlock(p, rect, seqblock, seqblocks==&_seqtrack->gfx_gfx_seqblocks);
          
        }
        
      }END_VECTOR_FOR_EACH;
    }


    // Current track border and non-current track shadowing
    {
      if (_seqtrack==(struct SeqTrack*)root->song->seqtracks.elements[root->song->curr_seqtracknum]){
        if (root->song->seqtracks.num_elements > 1){
          QPen pen(get_qcolor(SEQUENCER_CURRTRACK_BORDER_COLOR_NUM));
          //pen.setWidthF(1);
          p.setPen(pen);
          p.drawLine(t_x1, t_y1, t_x2, t_y1);
          p.drawLine(t_x1, t_y2, t_x2, t_y2);
        }
      }else{
        QColor color(0,0,0,35);
        //QColor color = get_qcolor(SEQUENCER_CURRTRACK_BORDER_COLOR_NUM);
        myFillRect(p, QRectF(t_x1,t_y1,width,height), color);
      }
    }

    
  }

  int _last_num_seqblocks = 0;
  int _last_num_undos = -1;
  
  void call_very_often(void){
    if (_last_num_seqblocks != _seqtrack->seqblocks.num_elements) {
      SEQUENCER_update();
      _last_num_seqblocks = _seqtrack->seqblocks.num_elements;
    }

    {
      int num_undos = Undo_num_undos();
      bool unequal_undos = _last_num_undos != num_undos;
      
      if (unequal_undos || _time.elapsed() > 5000) { // Update at least every five seconds.
        if (unequal_undos) {
          _last_num_undos = num_undos;

          if(false){
            _sequencer_widget->update();
          }else{
            double sample_rate = MIXER_get_sample_rate();
            double start_time = _start_time / sample_rate;
            double end_time = _end_time / sample_rate;
            
            // Not sure if this makes a difference (rather than just calling update()).
            VECTOR_FOR_EACH(struct SeqBlock *, seqblock, &_seqtrack->seqblocks){
              float x1 = get_seqblock_x1(seqblock, start_time, end_time);
              float x2 = get_seqblock_x2(seqblock, start_time, end_time);
              _sequencer_widget->update(x1-1, t_y1, x2-x1+1, height);
            }END_VECTOR_FOR_EACH;
          }
          
        } else {
          _sequencer_widget->update();
        }
        _time.restart();
      }
    }
  }
    
#if 0
  Seqblock_widget *get_seqblock_widget(int seqblocknum){
    if (seqblocknum >= _seqblock_widgets.size())
      create_seqblock_widgets();
  
    R_ASSERT_RETURN_IF_FALSE2(seqblocknum < _seqblock_widgets.size(), NULL);

    return _seqblock_widgets.at(seqblocknum);
  }
#endif
  
};




class Seqtrack_widget : public QWidget, public Ui::Seqtrack_widget {
  Q_OBJECT

 public:

  Seqblocks_widget _seqblocks_widget;
  SeqTrack *_seqtrack;

  Seqtrack_widget(QWidget *parent, SeqTrack *seqtrack, const double &start_time, const double &end_time)
    : QWidget(parent)
    , _seqblocks_widget(parent, seqtrack, start_time, end_time)
    , _seqtrack(seqtrack)
  {    
    setupUi(this);

    // Remove widget we don't use yet
#if 1
    header_widget->hide();
#endif
    
    //main_layout->addWidget(_seqblocks_widget);

    add_gc_root(_seqtrack);
  }

  ~Seqtrack_widget(){
    remove_gc_root(_seqtrack);
    printf("Seqtrack widget deleted\n");
  }



  float t_x1,t_y1,t_x2,t_y2,t_width,t_height;
  
  void position_widgets(float x1, float y1, float x2, float y2){
    t_x1 = x1;
    t_y1 = y1;
    t_x2 = x2;
    t_y2 = y2;
    t_height = t_y2-t_y1;
    t_width = t_x2-t_x1;
    
    float x1_b = x1;// + width();
    _seqblocks_widget.position_widgets(x1_b, y1, x2, y2);
  }

  void set_seqblocks_is_selected(const QRect &selection_rect){
    _seqblocks_widget.set_seqblocks_is_selected(selection_rect);
  }

  void paint(const QRect &update_rect, QPainter &p){
    _seqblocks_widget.paint(update_rect, p);
  }
  
  void my_update(void){
    //_seqblocks_widget.update();
  }

  void call_very_often(void){
    _seqblocks_widget.call_very_often();
  }

public slots:

};


namespace{

class Seqtracks_widget : public radium::VerticalScroll {
public:

  QWidget *_sequencer_widget;
  
  QVector<Seqtrack_widget*> _seqtrack_widgets;

  const double &_start_time;
  const double &_end_time;
  
  Seqtracks_widget(QWidget *sequencer_widget, const double &start_time, const double &end_time)
    : radium::VerticalScroll(sequencer_widget)
    , _sequencer_widget(sequencer_widget)
    ,_start_time(start_time)
    ,_end_time(end_time)
  {
    update_seqtracks();
    setContentsMargins(0,0,0,0);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }


  float t_x1=0,t_y1=0,t_x2=50,t_y2=50;
  float width=0,height=0;
  
  void position_widgets(float x1, float y1, float x2, float y2){
    t_x1 = x1;
    t_y1 = y1;
    t_x2 = x2;
    t_y2 = y2;
    width = t_x2-t_x1;
    height = t_y2-t_y1;

#if 0
    int num_seqtracks = _seqtrack_widgets.size();
    
    if (num_seqtracks > 5) {      
      //setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      for(auto *seqtrack_widget : _seqtrack_widgets){
        seqtrack_widget->setMinimumHeight(height / 5);
        seqtrack_widget->setMaximumHeight(height / 5);
      }
    }else {
      //setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      for(auto *seqtrack_widget : _seqtrack_widgets){
        seqtrack_widget->setMinimumHeight(height / 5);
        seqtrack_widget->setMaximumHeight(5000000);
      }
    }
#endif
    
    for(auto *seqtrack_widget : _seqtrack_widgets){
      float y1_b = y1+seqtrack_widget->y();
      float y2_b = y1_b + seqtrack_widget->height();
      seqtrack_widget->position_widgets(x1, y1_b, x2, y2_b);
    }

  }  

  void set_seqblocks_is_selected(const QRect &selection_rect){
    for(auto *seqtrack_widget : _seqtrack_widgets)
      seqtrack_widget->set_seqblocks_is_selected(selection_rect);
  }

  void paint(const QRect &update_rect, QPainter &p){
    position_widgets(t_x1, t_y1, t_x2, t_y2);

    // seqtracks
    //
    for(auto *seqtrack_widget : _seqtrack_widgets)
      seqtrack_widget->paint(update_rect, p);
  }

  void my_update(void){
    for(auto *seqtrack_widget : _seqtrack_widgets)
      seqtrack_widget->my_update();
    
    update();
  }

  void update_seqtracks(void){
    SEQUENCER_ScopedGfxDisable gfx_disable;
    
    //printf("  Updating seqtracks\n");
    
    for(auto *seqtrack_widget : _seqtrack_widgets)
      delete seqtrack_widget;
      
    _seqtrack_widgets.clear();
    
    VECTOR_FOR_EACH(struct SeqTrack *, seqtrack, &root->song->seqtracks){
      Seqtrack_widget *seqtrack_widget = new Seqtrack_widget(_sequencer_widget, seqtrack, _start_time, _end_time);
      _seqtrack_widgets.push_back(seqtrack_widget);
      addWidget(seqtrack_widget);
    }END_VECTOR_FOR_EACH;

    SEQUENCER_update(); // If we only call update(), the navigator won't update.
    //position_widgets(t_x1, t_y1, t_x2, t_y2);
  }

  
  void call_very_often(void){
    SEQUENCER_ScopedGfxDisable gfx_disable;
    
    if (_seqtrack_widgets.size() != root->song->seqtracks.num_elements)
      update_seqtracks();

    int i = 0;
    for(auto *seqtrack : _seqtrack_widgets){
      if (root->song->seqtracks.elements[i] != seqtrack->_seqtrack){
        update_seqtracks();
        call_very_often();
        return;
      }
      i++;

      seqtrack->call_very_often();
    }
  }

  Seqtrack_widget *get_seqtrack_widget(struct SeqTrack *seqtrack){
    for(auto *seqtrack_widget : _seqtrack_widgets)
      if (seqtrack_widget->_seqtrack==seqtrack)
        return seqtrack_widget;
    
    //R_ASSERT(false);    
    return NULL; // Think it may legitimately happen if gui hasn't been updated yet.
  }
  
  Seqtrack_widget *get_seqtrack_widget(int seqtracknum){
    if (seqtracknum >= _seqtrack_widgets.size())
      update_seqtracks();

    if (seqtracknum >= _seqtrack_widgets.size())
      return NULL;
    
    R_ASSERT_RETURN_IF_FALSE2(seqtracknum<_seqtrack_widgets.size(), NULL);

    //printf("%d: y: %d\n",seqtracknum,_seqtrack_widgets.at(seqtracknum)->pos().y());
    
    return _seqtrack_widgets.at(seqtracknum);
  }

};


struct SongTempoAutomation_widget { //: public MouseTrackerQWidget {
  bool is_visible = false;
   
  const double &_start_time;
  const double &_end_time;
   
  float t_x1,t_y1,t_x2,t_y2,width,height;
  QRectF _rect;
   
  SongTempoAutomation_widget(QWidget *parent, const double &start_time, const double &end_time)
  //: MouseTrackerQWidget(parent)
    : _start_time(start_time)
    , _end_time(end_time)
  {
    position_widgets(0,0,100,100);
  }

  void position_widgets(float x1, float y1, float x2, float y2){
    t_x1 = x1;
    t_y1 = y1;
    t_x2 = x2;
    t_y2 = y2;
    height = t_y2-t_y1;
    width = t_x2-t_x1;
    
    _rect = QRectF(t_x1, t_y1, width, height);
  }

  void paint(const QRect &update_rect, QPainter &p){

    myFillRect(p, _rect.adjusted(1,1,-2,-1), get_qcolor(SEQUENCER_BACKGROUND_COLOR_NUM));
    
    //printf("height: %d\n",height());
    TEMPOAUTOMATION_paint(&p, t_x1, t_y1, t_x2, t_y2, _start_time, _end_time);
  }

};

struct Timeline_widget : public MouseTrackerQWidget {
  const double &_start_time;
  const double &_end_time;
  
  Timeline_widget(QWidget *parent, const double &start_time, const double &end_time)
    :MouseTrackerQWidget(parent)
    ,_start_time(start_time)
    ,_end_time(end_time)
  {    
  }

  void wheelEvent(QWheelEvent *e) override {
    handle_wheel_event(e, 0, width(), _start_time, _end_time);
  }

  void draw_filled_triangle(QPainter &p, double x1, double y1, double x2, double y2, double x3, double y3){
    const QPointF points[3] = {
      QPointF(x1, y1),
      QPointF(x2, y2),
      QPointF(x3, y3)
    };

    p.drawPolygon(points, 3);
  }
  
  QString seconds_to_timestring(double time){
    return radium::get_time_string(time, false);
  }

  void paintEvent ( QPaintEvent * ev ) override {
    QPainter p(this);

    const double y1 = 4;
    const double y2 = height() - 4;
    const double t1 = (y2-y1) / 2.0;

    p.setRenderHints(QPainter::Antialiasing,true);

    QColor border_color = get_qcolor(SEQUENCER_BORDER_COLOR_NUM);
    //QColor text_color = get_qcolor(MIXER_TEXT_COLOR_NUM);

    QRect rect(1,1,width()-1,height()-2);
    p.fillRect(rect, get_qcolor(SEQUENCER_TIMELINE_BACKGROUND_COLOR_NUM));
    
    //p.setPen(text_color);
    //p.drawText(4,2,width()-6,height()-4, Qt::AlignLeft, "timeline");
    
    p.setPen(border_color);
    p.drawRect(rect);

    // This code is copied from hurtigmixer. (translated from scheme)

    QColor timeline_arrow_color = get_qcolor(SEQUENCER_TIMELINE_ARROW_COLOR_NUM);
    QColor timeline_arrow_color_alpha = timeline_arrow_color;
    timeline_arrow_color_alpha.setAlpha(20);

    p.setBrush(timeline_arrow_color);

    QColor text_color = get_qcolor(SEQUENCER_TEXT_COLOR_NUM);
    QColor text_color_alpha = text_color;
    text_color_alpha.setAlpha(20);
    
    p.setPen(text_color);
        
    const QFontMetrics fn = QFontMetrics(QApplication::font());
    const double min_pixels_between_text = fn.width("00:00:00") + t1*2 + 10;

      //double  = 40; //width() / 4;

    const double loop_start_x = scale_double(SEQUENCER_get_loop_start(), _start_time, _end_time, 0, width());
    const double loop_end_x = scale_double(SEQUENCER_get_loop_end(), _start_time, _end_time, 0, width());

    const double start_time = _start_time / MIXER_get_sample_rate();
    const double end_time = _end_time / MIXER_get_sample_rate();

    //if (end_time >= start_time)
    //  return;
    R_ASSERT_RETURN_IF_FALSE(end_time > start_time);
    
    int inc_time = R_MAX(1, ceil(scale(min_pixels_between_text, 0, width(), 0, end_time-start_time)));

    // Ensure inc_time is aligned in seconds, 5 seconds, or 30 seconds.
    {
      if (inc_time%2 != 0)
        inc_time++;
      
      if (inc_time%5 != 0)
        inc_time += 5-(inc_time%5);
      
      if ((end_time-start_time) > 110)
        if (inc_time%30 != 0)
          inc_time += 30-(inc_time%30);
      
      // Another time? (might be a copy and paste error)
      if ((end_time-start_time) > 110)
        if (inc_time%30 != 0)
          inc_time += 30-(inc_time%30);
    }
    //inc_time = 1;

    bool did_draw_alpha = false;
    
    int64_t time = inc_time * int((double)start_time/(double)inc_time);
    
    for(;;){
      const double x = scale_double(time, start_time, end_time, 0, width());
      if (x >= width())
        break;

      bool draw_alpha = false;

      if (loop_start_x >= x-t1 && loop_start_x <= x+t1+min_pixels_between_text)
        draw_alpha = true;
      if (loop_end_x >= x-t1 && loop_end_x <= x+t1+min_pixels_between_text)
        draw_alpha = true;

      if (draw_alpha==true && did_draw_alpha==false){
        p.setPen(text_color_alpha);
        p.setBrush(timeline_arrow_color_alpha);
      }

      if (draw_alpha==false && did_draw_alpha==true){
        p.setPen(text_color);
        p.setBrush(timeline_arrow_color);
      }

      did_draw_alpha = draw_alpha;
      
      if (x > 20) {
        
        draw_filled_triangle(p, x-t1, y1, x+t1, y1, x, y2);
        
        QRectF rect(x + t1 + 4, 2, width(), height());
        myDrawText(p, rect, seconds_to_timestring(time));
      }

      time += inc_time;
    }

    p.setPen(Qt::black);
    p.setBrush(QColor(0,0,255,150));

    // loop start
    {
      draw_filled_triangle(p, loop_start_x-t1*2, y1, loop_start_x, y1, loop_start_x, y2);
    }

    // loop end
    {
      draw_filled_triangle(p, loop_end_x, y1, loop_end_x+t1*2, y1, loop_end_x, y2);
    }
  }

};

struct Seqtracks_navigator_widget : public MouseTrackerQWidget {
  const double &_start_time;
  const double &_end_time;
  Seqtracks_widget &_seqtracks_widget;
  
  Seqtracks_navigator_widget(QWidget *parent, const double &start_time, const double &end_time, Seqtracks_widget &seqtracks_widget)
    : MouseTrackerQWidget(parent)
    , _start_time(start_time)
    , _end_time(end_time)
    , _seqtracks_widget(seqtracks_widget)
  {
  }

  void wheelEvent(QWheelEvent *e) override {
    handle_wheel_event(e, 0, width(), 0, SONG_get_gfx_length()*MIXER_get_sample_rate());
  }

private:
  float get_x1(double total){
    return scale(_start_time, 0, total, 0, width());
  }
  float get_x2(double total){
    return scale(_end_time, 0, total, 0, width());
  }
public:

  float get_x1(void){
    return get_x1(SONG_get_gfx_length()*MIXER_get_sample_rate());
  }
  float get_x2(void){
    return get_x2(SONG_get_gfx_length()*MIXER_get_sample_rate());
  }
  
  void paintEvent ( QPaintEvent * ev ) override {
    QPainter p(this);

    double total_seconds = SONG_get_gfx_length();
    double total = total_seconds*MIXER_get_sample_rate();
    
    p.setRenderHints(QPainter::Antialiasing,true);

    QColor border_color = get_qcolor(SEQUENCER_BORDER_COLOR_NUM);      
    

    // Background
    //
    QRect rect1(1,1,width()-1,height()-2);
    p.fillRect(rect1, get_qcolor(SEQUENCER_BACKGROUND_COLOR_NUM));

    
    // Blocks
    {

      //QColor block_color = QColor(140,140,140,180);
      QColor text_color = get_qcolor(SEQUENCER_TEXT_COLOR_NUM);
      
      int num_seqtracks = root->song->seqtracks.num_elements;
      int seqtracknum = 0;
      for(Seqtrack_widget *seqtrack_widget : _seqtracks_widget._seqtrack_widgets) {
        struct SeqTrack *seqtrack = seqtrack_widget->_seqtrack;
        
        float y1 = scale(seqtracknum,   0, num_seqtracks, 3, height()-3);
        float y2 = scale(seqtracknum+1, 0, num_seqtracks, 3, height()-3);
        
        SEQTRACK_update_all_seqblock_gfx_start_and_end_times(seqtrack);
        //double start_time = _start_time / MIXER_get_sample_rate();
        //double end_time = _end_time / MIXER_get_sample_rate();

        VECTOR_FOR_EACH(struct SeqBlock *, seqblock, &seqtrack->seqblocks){

          const struct Blocks *block = seqblock->block;
          
          QColor block_color = get_block_color(block);//->color);// = get_qcolor(SEQUENCER_BLOCK_BACKGROUND_COLOR_NUM);

          //printf("\n\n\n Start/end: %f / %f. Seqtrack/seqblock %p / %p\n\n", seqblock->start_time, seqblock->end_time, seqtrack, seqblock);
            
          float x1 = scale(seqblock->start_time, 0, total_seconds, 0, width()); //seqtrack_widget->_seqblocks_widget->get_seqblock_x1(seqblock, start_time, end_time);
          float x2 = scale(seqblock->end_time, 0, total_seconds, 0, width()); //seqtrack_widget->_seqblocks_widget->get_seqblock_x2(seqblock, start_time, end_time);
          
          QRectF rect(x1,y1+1,x2-x1,y2-y1-2);
          myFillRect(p, rect, block_color);

          if(rect.height() > root->song->tracker_windows->systemfontheight*1.3){
            p.setPen(text_color);
            //myDrawText(p, rect.adjusted(2,1,-1,-1), QString::number(block->l.num) + ": " + block->name);
            p.drawText(rect.adjusted(2,1,-1,-1), QString::number(block->l.num) + ": " + block->name, QTextOption(Qt::AlignLeft | Qt::AlignTop));
          }
          
          p.setPen(border_color);
          p.drawRect(rect);
          
        }END_VECTOR_FOR_EACH;
        
        seqtracknum++;
      }
    }


    // Navigator
    //
    {
      float x1 = get_x1(total);
      float x2 = get_x2(total);

      QRectF rectA(0,  1, x1,         height()-2);
      QRectF rectB(x2, 1, width()-x2, height()-2);      
      QRectF rect2(x1, 1, x2-x1,      height()-2);

      {
        QColor grayout_color = get_qcolor(SEQUENCER_NAVIGATOR_GRAYOUT_COLOR_NUM);
      
        p.fillRect(rectA, grayout_color);
        p.fillRect(rectB, grayout_color);
      }
      
      p.setPen(border_color);
      p.drawRect(rect2);
      
      float handle1_x = x1+SEQNAV_SIZE_HANDLE_WIDTH;
      float handle2_x = x2-SEQNAV_SIZE_HANDLE_WIDTH;
      //p.drawLine(handle1_x, 0, handle1, height());
      //p.drawLine(handle2_x, 0, handle1, height());
      
      QRectF handle1_rect(x1,        0, handle1_x-x1, height());
      QRectF handle2_rect(handle2_x, 0, x2-handle2_x, height());
      
      p.setBrush(get_qcolor(SEQUENCER_NAVIGATOR_HANDLER_COLOR_NUM));
      
      p.drawRect(handle1_rect);
      p.drawRect(handle2_rect);
    }
    
  }

};

struct Sequencer_widget : public MouseTrackerQWidget {

  int _old_width = 600;
  double _start_time = 0;
  double _end_time = 600;
  double _samples_per_pixel;

  enum GridType _grid_type;

  const int bottom_height = 30;
  SongTempoAutomation_widget _songtempoautomation_widget;
  Timeline_widget _timeline_widget;
  Seqtracks_widget _seqtracks_widget;
  Seqtracks_navigator_widget _navigator_widget;
  //MyQSlider _main_reltempo;

  bool _has_selection_rectangle = false;
  QRect _selection_rectangle;

  Sequencer_widget(QWidget *parent)
    : MouseTrackerQWidget(parent)
    , _end_time(SONG_get_gfx_length()*MIXER_get_sample_rate())
    , _samples_per_pixel((_end_time-_start_time) / width())
    , _songtempoautomation_widget(this, _start_time, _end_time)
    , _timeline_widget(this, _start_time, _end_time)
    , _seqtracks_widget(this, _start_time, _end_time)
    , _navigator_widget(this, _start_time, _end_time, _seqtracks_widget)
      //, _main_reltempo(this)
  {
    _timeline_widget.show();
    _seqtracks_widget.show();
    _navigator_widget.show();
    /*
    _main_reltempo.show();
    */

    int height = root->song->tracker_windows->systemfontheight*1.3 * 13;
    setMinimumHeight(height);
    setMaximumHeight(height);
  }

  void my_update(void){
    int64_t visible_song_length = MIXER_get_sample_rate() * SONG_get_gfx_length();
    if (_end_time > visible_song_length) {
      _end_time = visible_song_length;
      if (_start_time >= _end_time)
        _start_time = 0;
      if (_start_time >= _end_time)
        _end_time = 10;
    }
    
    _timeline_widget.update();
    _seqtracks_widget.my_update();
    _navigator_widget.update();
    update();
  }

  /*
  void set_end_time(void){
    _end_time = _start_time + width() * _samples_per_pixel;
    printf("End_time: %d\n",(int)_end_time);
  }
  */

  void wheelEvent(QWheelEvent *e) override {
    handle_wheel_event(e, 0, width(), _start_time, _end_time);
  }

  void resizeEvent( QResizeEvent *qresizeevent) override {
    //  set_end_time();
    // _samples_per_pixel = (_end_time-_start_time) / width();
    position_widgets();
  }
  
  void position_widgets(void){
    R_ASSERT_RETURN_IF_FALSE(_seqtracks_widget._seqtrack_widgets.size() > 0);

    //printf("   ***** Posisiotioing sequencer widgets ********\n");
    
#if 0
    const QWidget *mute_button = _seqtracks_widget._seqtrack_widgets.at(0)->mute_button;
    const QPoint p = mute_button->mapTo(this, mute_button->pos());
#endif
    
    const int x1 = 0; //p.x() + mute_button->width();
    const int x1_width = width() - x1;

    QFontMetrics fm(QApplication::font());
    float systemfontheight = fm.height();
      
    const int timeline_widget_height = systemfontheight*1.3 + 2;
 
    int y1 = 0;
    

    // timeline
    //
    _timeline_widget.setGeometry(x1, y1,
                                 x1_width, timeline_widget_height);


    y1 += timeline_widget_height;

    
    // song tempo automation
    //
    {
      const int songtempoautomation_widget_height = systemfontheight*1.3*3.5;

      int y2 = y1 + songtempoautomation_widget_height;

      _songtempoautomation_widget.position_widgets(1, y1,
                                                   width()-1, y2);

      if (_songtempoautomation_widget.is_visible)
        y1 = y2;
    }
    
    
    // sequencer tracks
    //
    {
      int y2 = height() - bottom_height;
      _seqtracks_widget.setGeometry(0, y1,
                                    1, y2 - y1);
      
      _seqtracks_widget.position_widgets(1,y1,
                                         width()-1, y2);

      y1 = y2;
    }


    // navigator
    //
    _navigator_widget.setGeometry(x1, y1,
                                  x1_width, bottom_height);

    /*
    _main_reltempo.setGeometry(0, y1,
                               x1, bottom_height);
    */

  }

  
  int _last_num_seqtracks = 0;
  double _last_visible_song_length = 0;
  
  const float cursor_width = 2.7;
  float _last_painted_cursor_x = 0.0f;
  
  float get_curr_cursor_x(int frames_to_add) const {
    if (is_playing() && pc->playtype==PLAYSONG && smooth_scrolling())
      return (_seqtracks_widget.t_x1 + _seqtracks_widget.t_x2) / 2.0;
    else
      return scale_double(ATOMIC_DOUBLE_GET(pc->song_abstime)+frames_to_add, _start_time, _end_time, _seqtracks_widget.t_x1, _seqtracks_widget.t_x2);
  }

  bool _song_tempo_automation_was_visible = false;

  bool _was_playing_smooth_song = false;

  void call_very_often(void){

    if (_song_tempo_automation_was_visible != _songtempoautomation_widget.is_visible){
      _song_tempo_automation_was_visible = _songtempoautomation_widget.is_visible;
      position_widgets();
    }
    
    if (is_called_every_ms(15)){  // call each 15 ms. (i.e. more often than vsync)
      _seqtracks_widget.call_very_often();    
      if (g_need_update){
        my_update();
        BS_UpdatePlayList();
        g_need_update=false;
      }
    }

    // Check if the number of seqtracks have changed
    //
    if (is_called_every_ms(50)){
      bool do_update = _seqtracks_widget._seqtrack_widgets.size() != _last_num_seqtracks;
      
      if (!do_update) {
        if (is_called_every_ms(1000)) // This is only an insurance. SEQUENCER_update is supposed to be called manually when needed.
          if (_last_visible_song_length != SONG_get_gfx_length()) {
            do_update = true;
            _last_visible_song_length = SONG_get_gfx_length();
          }
      }  
      
      if (do_update){
        //position_widgets();
        my_update();
        _last_num_seqtracks = _seqtracks_widget._seqtrack_widgets.size();
      }
    }

    // Update cursor
    //
    if (is_called_every_ms(15)){  // call each 15 ms. (i.e. more often than vsync)
      if (is_playing() && pc->playtype==PLAYSONG) {

        double song_abstime = ATOMIC_DOUBLE_GET(pc->song_abstime);
        double middle = (_start_time+_end_time) / 2.0;
        
        if (!smooth_scrolling()){
          float x = get_curr_cursor_x(1 + MIXER_get_sample_rate() * 60.0 / 1000.0);
          
          float x_min = R_MIN(x-cursor_width/2.0, _last_painted_cursor_x-cursor_width/2.0) - 2;
          float x_max = R_MAX(x+cursor_width/2.0, _last_painted_cursor_x+cursor_width/2.0) + 2;
          
          //printf("x_min -> x_max: %f -> %f\n",x_min,x_max);
          float y1 = _songtempoautomation_widget.t_y1;
          float y2 = _seqtracks_widget.t_y2;
          
          update(x_min, y1, 1+x_max-x_min, y2-y1);
          
          if (song_abstime < _start_time) {
            int64_t diff = _start_time - song_abstime;
            _start_time -= diff;
            _end_time -= diff;
            update();
          } else if (song_abstime > _end_time){
            double diff = song_abstime - middle;
            _start_time += diff;
            _end_time += diff;
            update();
          }

        } else {
          
          // Smooth scrolling

          _was_playing_smooth_song = true;

          if (song_abstime != middle){
            double diff = song_abstime - middle;
            _start_time += diff;
            _end_time += diff;
            update();
          }

          return;

        }
      }

      if (_was_playing_smooth_song==true){
        if (_start_time < 0){
          _end_time -= _start_time;
          _start_time = 0;
        }
        update();
        _was_playing_smooth_song = false;
      }
    }
  }

  void set_seqblocks_is_selected(void){
    _seqtracks_widget.set_seqblocks_is_selected(_selection_rectangle);
  }


  void paintGrid(const QRect &update_rect, QPainter &p, enum GridType grid_type) const {
    float x1 = _seqtracks_widget.t_x1;
    float x2 = _seqtracks_widget.t_x2;
    float width = x2-x1;
    
    float y1 = _songtempoautomation_widget.t_y1;
    float y2 = _seqtracks_widget.t_y2;

    if (grid_type==BAR_GRID) {
      QPen pen(get_qcolor(SEQUENCER_GRID_COLOR_NUM));
      p.setPen(pen);
      int64_t last_bar = -50000;
      int64_t abstime = _start_time;
      int inc = (_end_time-_start_time) / width;
      while(abstime < _end_time){
        int64_t maybe = SEQUENCER_find_closest_bar_start(0, abstime);
        if (maybe > last_bar){
          float x = scale(maybe, _start_time, _end_time, 0, width);
          //printf("x: %f, abstime: %f\n",x,(float)maybe/44100.0);
          QLineF line(x, y1+2, x, y2-4);
          p.drawLine(line);
          last_bar = maybe;
          abstime = maybe;
        }
        abstime += inc;
      }
    }
  }
      
  void paintCursor(const QRect &update_rect, QPainter &p){
    float y1 = _songtempoautomation_widget.t_y1;
    float y2 = _seqtracks_widget.t_y2;
    
    QPen pen(get_qcolor(SEQUENCER_CURSOR_COLOR_NUM));
    pen.setWidthF(cursor_width);
    
    _last_painted_cursor_x = get_curr_cursor_x(0);
    
    QLineF line(_last_painted_cursor_x, y1+2, _last_painted_cursor_x, y2-4);
    
    p.setPen(pen);
    p.drawLine(line);
  }
      
  void paintSeqloop(const QRect &update_rect, QPainter &p) const {
    float y1 = _songtempoautomation_widget.t_y1;
    float y2 = _seqtracks_widget.t_y2;

    float x_start = scale_double(SEQUENCER_get_loop_start(), _start_time, _end_time, 0, width());
    float x_end = scale_double(SEQUENCER_get_loop_end(), _start_time, _end_time, 0, width());

    QColor grayout_color = get_qcolor(SEQUENCER_NAVIGATOR_GRAYOUT_COLOR_NUM);

    p.setPen(Qt::NoPen);
    p.setBrush(grayout_color);

    if (x_start > 0){
      QRectF rect(0, y1, x_start,  y2);
      p.drawRect(rect);
    }

    if (x_end < width()){
      QRectF rect(x_end, y1, width(),  y2);
      p.drawRect(rect);
    }
  }
      
  void paintSelectionRectangle(const QRect &update_rect, QPainter &p) const {
    QColor grayout_color = QColor(220,220,220,0x40); //get_qcolor(SEQUENCER_NAVIGATOR_GRAYOUT_COLOR_NUM);

    p.setPen(Qt::black);
    p.setBrush(grayout_color);

    p.drawRect(_selection_rectangle);
  }

  void paintEvent (QPaintEvent *ev) override {
    QPainter p(this);

    p.setRenderHints(QPainter::Antialiasing,true);    

    _seqtracks_widget.paint(ev->rect(), p);

    if (_songtempoautomation_widget.is_visible)
      _songtempoautomation_widget.paint(ev->rect(), p);

    paintGrid(ev->rect(), p, _grid_type);
    paintCursor(ev->rect(), p);
    if (SEQUENCER_is_looping())
      paintSeqloop(ev->rect(), p);    

    if (_has_selection_rectangle)
      paintSelectionRectangle(ev->rect(), p);
  }

  
  Seqtrack_widget *get_seqtrack_widget(struct SeqTrack *seqtrack){
    return _seqtracks_widget.get_seqtrack_widget(seqtrack);
  }
  
  Seqtrack_widget *get_seqtrack_widget(int seqtracknum){
    return _seqtracks_widget.get_seqtrack_widget(seqtracknum);
  }

};


}

static Sequencer_widget *g_sequencer_widget = NULL;

static void g_position_widgets(void){
  if (g_sequencer_widget != NULL)
    g_sequencer_widget->position_widgets();
}

// sequencer

float SEQUENCER_get_x1(void){
  return mapToEditorX1(g_sequencer_widget);
}

float SEQUENCER_get_x2(void){
  return mapToEditorX2(g_sequencer_widget);
}

float SEQUENCER_get_y1(void){
  return mapToEditorY1(g_sequencer_widget);
}

float SEQUENCER_get_y2(void){
  return mapToEditorY2(g_sequencer_widget);
}


static int update_enabled_counter = 0;

void SEQUENCER_disable_gfx_updates(void){
  update_enabled_counter++;

  // Commented out since it didn't make a difference, plus that the setUpdatesEnabled() function takes a while to execute
  /*
  if (update_enabled_counter==1 && g_sequencer_widget!=NULL)
    g_sequencer_widget->setUpdatesEnabled(false);
  */
}
void SEQUENCER_enable_gfx_updates(void){
  update_enabled_counter--;

  // Commented out since it didn't make a difference, plus that the setUpdatesEnabled() function takes a while to execute
  /*
  if (update_enabled_counter==0 && g_sequencer_widget!=NULL)
    g_sequencer_widget->setUpdatesEnabled(true);
  */
}

int64_t SEQUENCER_get_visible_start_time(void){
  return g_sequencer_widget->_start_time;
}

int64_t SEQUENCER_get_visible_end_time(void){
  return g_sequencer_widget->_end_time;
}

void SEQUENCER_set_visible_start_and_end_time(int64_t start_time, int64_t end_time){
  R_ASSERT_RETURN_IF_FALSE(end_time > start_time);
  
  g_sequencer_widget->_start_time = R_MAX(start_time, 0);
  g_sequencer_widget->_end_time = R_MIN(end_time, SONG_get_gfx_length() * MIXER_get_sample_rate());

  if (g_sequencer_widget->_start_time >= g_sequencer_widget->_end_time - 10)
    g_sequencer_widget->_start_time = R_MAX(g_sequencer_widget->_end_time - 10, 0);
    
  g_sequencer_widget->my_update();
}

void SEQUENCER_set_visible_start_time(int64_t val){
  R_ASSERT_RETURN_IF_FALSE(val < g_sequencer_widget->_end_time);

  g_sequencer_widget->_start_time = R_MAX(val, 0);
  g_sequencer_widget->my_update();
}

void SEQUENCER_set_visible_end_time(int64_t val){
  R_ASSERT_RETURN_IF_FALSE(val > g_sequencer_widget->_start_time);
  
  g_sequencer_widget->_end_time = R_MIN(val, SONG_get_gfx_length() * MIXER_get_sample_rate());
  g_sequencer_widget->my_update();
}

void SEQUENCER_set_grid_type(enum GridType grid_type){
  g_sequencer_widget->_grid_type = grid_type;
}

void SEQUENCER_set_selection_rectangle(float x1, float y1, float x2, float y2){
  g_sequencer_widget->_has_selection_rectangle = true;
  QPoint p1 = mapFromEditor(g_sequencer_widget, QPoint(x1, y1));
  QPoint p2 = mapFromEditor(g_sequencer_widget, QPoint(x2, y2));

  // legalize values
  //
  int &_x1 = p1.rx();
  int &_y1 = p1.ry();
  int &_x2 = p2.rx();
  int &_y2 = p2.ry();

  if (_x1 < g_sequencer_widget->_seqtracks_widget.t_x1)
    _x1 = g_sequencer_widget->_seqtracks_widget.t_x1;
  if (_x2 < _x1)
    _x2 = _x1;

  if (_x2 > g_sequencer_widget->_seqtracks_widget.t_x2)
    _x2 = g_sequencer_widget->_seqtracks_widget.t_x2;
  if (_x1 > _x2)
    _x1 = _x2;

  if (_y1 < g_sequencer_widget->_seqtracks_widget.t_y1)
    _y1 = g_sequencer_widget->_seqtracks_widget.t_y1;
  if (_y2 < _y1)
    _y2 = _y1;

  if (_y2 > g_sequencer_widget->_seqtracks_widget.t_y2)
    _y2 = g_sequencer_widget->_seqtracks_widget.t_y2;
  if (_y1 > _y2)
    _y1 = _y2;


  g_sequencer_widget->_selection_rectangle = QRect(p1, p2);

  g_sequencer_widget->set_seqblocks_is_selected();

  g_sequencer_widget->update();
}

void SEQUENCER_unset_selection_rectangle(void){
  g_sequencer_widget->_has_selection_rectangle = false;

  g_sequencer_widget->update();
}


// sequencer navigator

float SEQNAV_get_x1(void){
  return mapToEditorX1(&g_sequencer_widget->_navigator_widget);
}

float SEQNAV_get_x2(void){
  return mapToEditorX2(&g_sequencer_widget->_navigator_widget);
}

float SEQNAV_get_y1(void){
  return mapToEditorY1(&g_sequencer_widget->_navigator_widget);
}

float SEQNAV_get_y2(void){
  return mapToEditorY2(&g_sequencer_widget->_navigator_widget);
}

float SEQNAV_get_left_handle_x(void){
  return SEQNAV_get_x1() + g_sequencer_widget->_navigator_widget.get_x1();
}

float SEQNAV_get_right_handle_x(void){
  return SEQNAV_get_x1() + g_sequencer_widget->_navigator_widget.get_x2();
}

void SEQNAV_update(void){
  g_sequencer_widget->_navigator_widget.update();
}



// sequencer looping

float SEQTIMELINE_get_x1(void){
  return mapToEditorX1(&g_sequencer_widget->_timeline_widget);
}

float SEQTIMELINE_get_x2(void){
  return mapToEditorX2(&g_sequencer_widget->_timeline_widget);
}

float SEQTIMELINE_get_y1(void){
  return mapToEditorY1(&g_sequencer_widget->_timeline_widget);
}

float SEQTIMELINE_get_y2(void){
  return mapToEditorY2(&g_sequencer_widget->_timeline_widget);
}




// seqtempo automation

float SEQTEMPO_get_x1(void){
  return mapToEditorX(g_sequencer_widget, 0);
  //return mapToEditorX1(&g_sequencer_widget->_songtempoautomation_widget);
}

float SEQTEMPO_get_x2(void){
  return mapToEditorX(g_sequencer_widget, g_sequencer_widget->width());
  //return mapToEditorX2(&g_sequencer_widget->_songtempoautomation_widget);
}

float SEQTEMPO_get_y1(void){
  return mapToEditorY1(g_sequencer_widget) + g_sequencer_widget->_songtempoautomation_widget.t_y1;
}

float SEQTEMPO_get_y2(void){
  return mapToEditorY1(g_sequencer_widget) + g_sequencer_widget->_songtempoautomation_widget.t_y2;
}

void SEQTEMPO_set_visible(bool visible){
  if (g_sequencer_widget != NULL){
    g_sequencer_widget->_songtempoautomation_widget.is_visible = visible;
    g_sequencer_widget->position_widgets();
    SEQUENCER_update();
  }
}

bool SEQTEMPO_is_visible(void){
  return g_sequencer_widget->_songtempoautomation_widget.is_visible;
}


// seqblocks

float SEQBLOCK_get_x1(int seqblocknum, int seqtracknum){
  auto *w0 = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  if (w0==NULL)
    return 0.0;
  
  const auto &w = w0->_seqblocks_widget;

  return mapToEditorX(g_sequencer_widget, 0) + w.get_seqblock_x1(seqblocknum);
}

float SEQBLOCK_get_x2(int seqblocknum, int seqtracknum){
  auto *w0 = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  if (w0==NULL)
    return 0.0;
  
  const auto &w = w0->_seqblocks_widget;

  return mapToEditorX(g_sequencer_widget, 0) + w.get_seqblock_x2(seqblocknum);
}

float SEQBLOCK_get_y1(int seqblocknum, int seqtracknum){
  auto *w0 = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  if (w0==NULL)
    return 0.0;
  
  const auto &w = w0->_seqblocks_widget;

  return mapToEditorY(g_sequencer_widget, w.t_y1);
}

float SEQBLOCK_get_y2(int seqblocknum, int seqtracknum){
  auto *w0 = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  if (w0==NULL)
    return 0.0;
  
  const auto &w = w0->_seqblocks_widget;

  return mapToEditorY(g_sequencer_widget, w.t_y2);
}



// seqtracks

float SEQTRACK_get_x1(int seqtracknum){
  //auto *w = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  //if (w==NULL)
  //  return 0.0;
    
  return mapToEditorX(g_sequencer_widget, 0);
}

float SEQTRACK_get_x2(int seqtracknum){
  //auto *w = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  //if (w==NULL)
  //  return 0.0;
    
  return mapToEditorX(g_sequencer_widget, g_sequencer_widget->width());
}

float SEQTRACK_get_y1(int seqtracknum){
  Seqtrack_widget *w = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  if (w==NULL)
    return 0.0;
    
  return mapToEditorY(w, 0);
}

float SEQTRACK_get_y2(int seqtracknum){
  auto *w = g_sequencer_widget->get_seqtrack_widget(seqtracknum);
  if (w==NULL)
    return 0.0;
    
  return mapToEditorY(w, w->height());
}

void SEQTRACK_update(struct SeqTrack *seqtrack){
  if (g_sequencer_widget == NULL)
    return;

  //g_sequencer_widget->update();
  //return;
  
  Seqtrack_widget *w = g_sequencer_widget->get_seqtrack_widget(seqtrack);
  if (w==NULL)
    return;
  
  g_sequencer_widget->update(0, w->t_y1,
                             g_sequencer_widget->width(), w->t_height);
}

void SEQUENCER_update(void){
  if (g_sequencer_widget != NULL){
    //g_sequencer_widget->position_widgets();
    g_sequencer_widget->update();
  }
}

// Only called from the main thread.
void RT_SEQUENCER_update_sequencer_and_playlist(void){
  R_ASSERT_RETURN_IF_FALSE(THREADING_is_main_thread());

  if (PLAYER_current_thread_has_lock()) {
    
    g_need_update = true;
    
  } else {

    SEQUENCER_update();
    BS_UpdatePlayList();
    
    g_need_update=false;
    
  }
}

static bool g_sequencer_visible = true;
static bool g_sequencer_hidden_because_instrument_widget_is_large = false;

static void init_sequencer_visible(void){
  static bool g_sequencer_visible_inited = false;
  if (g_sequencer_visible_inited==false){
    g_sequencer_visible = g_sequencer_widget->isVisible();
    g_sequencer_visible_inited=true;
  }
}

bool GFX_SequencerIsVisible(void){
  init_sequencer_visible();
  return g_sequencer_visible;
}

void GFX_ShowSequencer(void){
  init_sequencer_visible();
  
  //set_widget_height(30);
  if (g_sequencer_hidden_because_instrument_widget_is_large == false){
    GL_lock(); {
      g_sequencer_widget->show();
      g_sequencer_visible = true;
    }GL_unlock();
  }

  set_editor_focus();
}

void GFX_HideSequencer(void){
  init_sequencer_visible();
  
  g_sequencer_widget->hide();
  g_sequencer_visible = false;
  //set_widget_height(0);

  set_editor_focus();
}

void SEQUENCER_hide_because_instrument_widget_is_large(void){
  init_sequencer_visible();

  g_sequencer_widget->hide();
  g_sequencer_hidden_because_instrument_widget_is_large = true;
}

void SEQUENCER_show_because_instrument_widget_is_large(void){
  init_sequencer_visible();
  
  if (g_sequencer_visible == true){
    GL_lock(); {
      g_sequencer_widget->show();
    }GL_unlock();
  }

  g_sequencer_hidden_because_instrument_widget_is_large = false; 
}

bool SEQUENCER_has_mouse_pointer(void){
  if (g_sequencer_widget->isVisible()==false)
    return false;

  QPoint p = g_sequencer_widget->mapFromGlobal(QCursor::pos());
  if (true
      && p.x() >= 0
      && p.x() <  g_sequencer_widget->width()
      && p.y() >= 0
      && p.y() <  g_sequencer_widget->height()
      )
    return true;
  else
    return false;
}
