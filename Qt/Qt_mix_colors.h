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

#ifndef QT_MIX_COLORS_H
#define QT_MIX_COLORS_H

#include <math.h>

static inline QColor mix_colors(const QColor &c1, const QColor &c2, float how_much){

  float a1 = how_much;
  float a2 = 1.0f-a1;

  if(c1.red()==0 && c1.green()==0 && c1.blue()==0){ // some of the black lines doesn't look look very good.
    int r = 74*a1 + c2.red()*a2;
    int g = 74*a1 + c2.green()*a2;
    int b = 74*a1 + c2.blue()*a2;

    return QColor(r,g,b);
  }else{

    int r = sqrtf(c1.red()*c1.red()*a1      + c2.red()*c2.red()*a2);
    int g = sqrtf(c1.green()*c1.green()*a1  + c2.green()*c2.green()*a2);
    int b = sqrtf(c1.blue()*c1.blue()*a1    + c2.blue()*c2.blue()*a2);

    return QColor(r,g,b);
  }
}

#endif
