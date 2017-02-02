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

#ifndef _STRING_INDEX__
#define _STRING_INDEX__

#include <cassert>
#include <string>
using std::string;
#include <map>
using std::map;

class StringIndex{

 private:

  // Data members...
  map <string, int> msi;
  map <int, string> mis;
  int maxIndex;

 public:

  // Constructor / destructor methods...
  StringIndex() { maxIndex=0; }

  // Specification methods...
  int addIndex (const char* ps)  { assert(ps!=NULL); return addIndex(string(ps)); }
  int addIndex (const string& s) { if(msi.end()==msi.find(s)){msi[s]=maxIndex;mis[maxIndex]=s;maxIndex++;} return msi[s]; }
  void clear   ( )               { msi.clear(); mis.clear(); maxIndex=0; }
  // Extraction methods...
  int           getSize   ( )               const { return maxIndex; }
  int           getIndex  (const char* ps)  const { assert(ps!=NULL); return getIndex(string(ps)); }
  int           getIndex  (const string& s) const { assert(msi.end()!=msi.find(s));
                                                    return msi.find(s)->second; }
  const string& getString (int i)           const { assert(mis.end()!=mis.find(i));
                                                    return mis.find(i)->second; }
};


#endif // _STRING_INDEX__
