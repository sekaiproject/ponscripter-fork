/* -*- C++ -*-
 * 
 *  DirtyRect.cpp - Invalid region on text_surface which should be updated
 *
 *  Copyright (c) 2001-2004 Ogapee. All rights reserved.
 *
 *  ogapee@aqua.dti2.ne.jp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "DirtyRect.h"

DirtyRect::DirtyRect()
{
    area = 0;
    total_history = 10;
    num_history = 0;
    bounding_box.w = bounding_box.h = 0;
    history = new SDL_Rect[total_history];
};

DirtyRect::DirtyRect( const DirtyRect &d )
{
    area = d.area;
    total_history = d.total_history;
    num_history = d.num_history;
    bounding_box = d.bounding_box;
    history = new SDL_Rect[total_history];

    for ( int i=0 ; i<num_history ; i++ )
        history[i] = d.history[i];
};

DirtyRect& DirtyRect::operator =( const DirtyRect &d )
{
    area = d.area;
    total_history = d.total_history;
    num_history = d.num_history;
    bounding_box = d.bounding_box;
    delete[] history;
    history = new SDL_Rect[total_history];

    for ( int i=0 ; i<num_history ; i++ )
        history[i] = d.history[i];

    return *this;
};

DirtyRect::~DirtyRect()
{
    delete[] history;
}

void DirtyRect::add( SDL_Rect src )
{
    //printf("add %d %d %d %d\n", src.x, src.y, src.w, src.h );
    if ( src.w == 0 || src.h == 0 ) return;

    if (src.x < 0){
        if (src.w < -src.x) return;
        src.w += src.x;
        src.x = 0;
    }
    if (src.y < 0){
        if (src.h < -src.y) return;
        src.h += src.y;
        src.y = 0;
    }

    bounding_box = calcBoundingBox( bounding_box, src );

    int i, min_is=0, min_hist=-1;
    for (i=0 ; i<num_history ; i++){
        SDL_Rect rect = calcBoundingBox(src, history[i]);
        if (i==0 || rect.w*rect.h < min_is){
            min_is = rect.w*rect.h;
            min_hist = i;
        }
    }
    if (min_hist >= 0 &&
        history[min_hist].w * history[min_hist].h + src.w * src.h > min_is ){
        area -= history[min_hist].w * history[min_hist].h;
        history[min_hist] = calcBoundingBox(src, history[min_hist]);
        area += history[min_hist].w * history[min_hist].h;
        return;
    }
    
    history[ num_history++ ] = src;
    area += src.w * src.h;
    
    if ( num_history == total_history ){
        total_history += 10;
        SDL_Rect *tmp = history;
        history = new SDL_Rect[ total_history ];
        for ( i=0 ; i<num_history ; i++ )
            history[i] = tmp[i];

        delete[] tmp;
    }
};

SDL_Rect DirtyRect::calcBoundingBox( SDL_Rect src1, SDL_Rect &src2 )
{
    if ( src2.w == 0 || src2.h == 0 ){
        return src1;
    }
    if ( src1.w == 0 || src1.h == 0 ){
        src1 = src2;
        return src1;
    }

    if ( src1.x > src2.x ){
        src1.w += src1.x - src2.x;
        src1.x = src2.x;
    }
    if ( src1.y > src2.y ){
        src1.h += src1.y - src2.y;
        src1.y = src2.y;
    }
    if ( src1.x + src1.w < src2.x + src2.w ){
        src1.w = src2.x + src2.w - src1.x;
    }
    if ( src1.y + src1.h < src2.y + src2.h ){
        src1.h = src2.y + src2.h - src1.y;
    }

    return src1;
}

void DirtyRect::clear()
{
    area = 0;
    num_history = 0;
    bounding_box.w = bounding_box.h = 0;
}

void DirtyRect::fill( int w, int h )
{
    area = w*h;
    bounding_box.x = bounding_box.y = 0;
    bounding_box.w = w;
    bounding_box.h = h;
}
