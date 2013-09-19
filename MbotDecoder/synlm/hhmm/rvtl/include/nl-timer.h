///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// This file is part of ModelBlocks. Copyright 2009, ModelBlocks developers. //
//                                                                           //
//    ModelBlocks is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by   //
//    the Free Software Foundation, either version 3 of the License, or      //
//    (at your option) any later version.                                    //
//                                                                           //
//    ModelBlocks is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of         //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          //
//    GNU General Public License for more details.                           //
//                                                                           //
//    You should have received a copy of the GNU General Public License      //
//    along with ModelBlocks.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                           //
//    ModelBlocks developers designate this particular file as subject to    //
//    the "Moses" exception as provided by ModelBlocks developers in         //
//    the LICENSE file that accompanies this code.                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef _NL_TIMER__
#define _NL_TIMER__

#include <sys/time.h>

class Timer {
 private:
  struct timeval kept;
  struct timeval beg;
 public:
  Timer ( ) { gettimeofday(&beg,NULL); kept.tv_sec=0; kept.tv_usec=0; }
  void start ( ) { gettimeofday(&beg,NULL); }
  void pause ( ) {
    struct timeval now; gettimeofday(&now,NULL);
    kept.tv_sec  += now.tv_sec  - beg.tv_sec;
    kept.tv_usec += (now.tv_usec - beg.tv_usec)%1000000;
    kept.tv_sec  += int((now.tv_usec - beg.tv_usec)/1000000);
  }
  double elapsed ( ) {  // in milliseconds.
    return (double(kept.tv_sec)*1000.0 + double(kept.tv_usec)/1000.0);
    //struct timeval end; gettimeofday(&end,NULL); 
    //double beg_time_s = (double) beg.tv_sec + (double) ((double)beg.tv_usec / 1000000.0);
    //double end_time_s = (double) end.tv_sec + (double) ((double)end.tv_usec / 1000000.0);
    //return ( (end_time_s - beg_time_s) * 1000.0 );
  }
};

#endif //_NL_TIMER__

