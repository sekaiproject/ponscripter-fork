/* -*- C++ -*-
 * 
 *  DirPaths.cpp - contains multiple directory paths
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

#include "BaseReader.h"
#include "DirPaths.h"

DirPaths::DirPaths()
{
    paths.clear();
}

DirPaths::DirPaths( const char& new_paths )
{
    paths.clear();
    pstring tmp = new_paths;
    add(tmp);
}

DirPaths::DirPaths( const pstring& new_paths )
{
    paths.clear();
    add(new_paths);
}

DirPaths::DirPaths( DirPaths& old_dp )
{
    paths.clear();
    add(old_dp.get_all_paths());
}

DirPaths::~DirPaths()
{
    paths.clear();
}

void DirPaths::clear()
{
    paths.clear();
}

void DirPaths::add( const pstring& new_paths )
{
    if (new_paths.length()==0) return;

    pstring paths_tmp = new_paths;
    char *ptr1, *ptr2;
    ptr1 = ptr2 = paths_tmp.mutable_data();
    fprintf(stderr, "Adding: %s\n", ptr1);

    do {
        while ((*ptr2 != '\0') && (*ptr2 != PATH_DELIMITER)) ptr2++;
        if (ptr2 == ptr1) {
            if (*ptr2 == '\0') break;
            ptr1++;
            ptr2++;
            continue;
        } else {
            char dummy = *ptr2;
            *ptr2 = '\0';
            pstring tmp = ptr1;
            *ptr2 = dummy;
            if (tmp.length() > 0) {
                if (tmp.midstr(tmp.length()-1,1) != DELIMITER) {
                    // put a slash on the end if there isn't one already
                    tmp += DELIMITER;
                }
                unsigned int i;
                for (i=0; i<paths.size(); i++)
                    if (tmp == paths[i]) break;
                if (i == paths.size())
                    paths.push_back(tmp);
            }
        }
        if (*ptr2 != '\0') {
            ptr1++;
            ptr2++;
        }
    } while (*ptr2 != '\0');
}

pstring DirPaths::get_path( int n )
{
    pstring dummy("");
    if ((n >= (int)paths.size()) || (n < 0))
        return dummy;
    else
        return paths.at(n);
}

// Returns a delimited string containing all paths
pstring DirPaths::get_all_paths()
{
    pstring all_paths("");
    unsigned int n;
    for (n=0; n<paths.size()-1; n++) {
        all_paths += paths.at(n) + PATH_DELIMITER;
    }
    all_paths += paths.at(n);

    return all_paths;
}

int DirPaths::get_num_paths()
{
    return paths.size();
}

// Returns the length of the longest path
unsigned int DirPaths::max_path_len()
{
    int len = 0;
    for (unsigned int i=0; i<paths.size();i++) {
        if (paths.at(i).length() > len)
            len = paths.at(i).length();
    }
    return (unsigned int) len;
}
