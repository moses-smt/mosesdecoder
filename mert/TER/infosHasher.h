/*********************************
tercpp: an open-source Translation Edit Rate (TER) scorer tool for Machine Translation.

Copyright 2010-2013, Christophe Servan, LIUM, University of Le Mans, France
Contact: christophe.servan@lium.univ-lemans.fr

The tercpp tool and library are free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the licence, or
(at your option) any later version.

This program and library are distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**********************************/
#ifndef __INFOSHASHER_H__
#define __INFOSHASHER_H__
#include <string>
// #include <ext/hash_map>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include "tools.h"

using namespace std;
namespace TERCPPNS_HashMapSpace
{
class infosHasher
{
private:
  long m_hashKey;
  string m_key;
  vector<int> m_value;

public:
  infosHasher ( long cle, string cleTxt, vector<int> valueVecInt );
  long getHashKey();
  string getKey();
  vector<int> getValue();
  void setValue ( vector<int> value );
  string toString();


};


}
#endif
