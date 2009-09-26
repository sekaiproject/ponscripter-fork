/* -*- C++ -*-
 * 
 *  DirPaths.h - contains multiple directory paths
 *
 *  Adapted by Mion from code added to ONScripter-EN in Nov. 2007
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

#ifndef __DIR_PATHS__
#define __DIR_PATHS__

#include "defs.h"

#ifdef WIN32
#define PATH_DELIMITER ';'
#else
#define PATH_DELIMITER ':'
#endif

class DirPaths
{
public:
    DirPaths();
    DirPaths( const char& new_paths );
    DirPaths( const pstring& new_paths );
    DirPaths( DirPaths& old_dp );
    ~DirPaths();
    
    void clear();
    void add( const pstring& new_paths );
    pstring get_path( int n );
    pstring get_all_paths();
    int get_num_paths();
    unsigned int max_path_len();

private:
    std::deque<pstring> paths;
};

#endif // __DIR_PATHS__
