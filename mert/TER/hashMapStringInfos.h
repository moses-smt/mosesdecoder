/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/
/*
 * Generic hashmap manipulation functions
 */
#ifndef __HASHMAPSTRINGINFOS_H_
#define __HASHMAPSTRINGINFOS_H_
#include <boost/functional/hash.hpp>
#include "stringInfosHasher.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

namespace HashMapSpace
{
class hashMapStringInfos
{
private:
  vector<stringInfosHasher> m_hasher;

public:
//     ~hashMap();
  long hashValue ( string key );
  int trouve ( long searchKey );
  int trouve ( string key );
  void addHasher ( string key, vector<string>  value );
  void addValue ( string key, vector<string>  value );
  stringInfosHasher getHasher ( string key );
  vector<string> getValue ( string key );
//         string searchValue ( string key );
  void setValue ( string key , vector<string>  value );
  void printHash();
  string toString();
  vector<stringInfosHasher> getHashMap();
  string printStringHash();
  string printStringHash2();
  string printStringHashForLexicon();
};


}


#endif
