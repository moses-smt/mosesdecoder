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
#ifndef __TERCPPTERSHIFT_H__
#define __TERCPPTERSHIFT_H__


#include <vector>
#include <cstdio>
#include <string>
#include <sstream>
#include "tools.h"


using namespace std;
using namespace TERCPPNS_Tools;

namespace TERCPPNS_TERCpp
{
class terShift
{
private:
public:

  terShift();
  terShift ( int _start, int _end, int _moveto, int _newloc );
  terShift ( int _start, int _end, int _moveto, int _newloc, vector<string> _shifted );
  string toString();
  int distance() ;
  bool leftShift();
  int size();
// 	terShift operator=(terShift t);
// 	string vectorToString(vector<string> vec);

  int start;
  int end;
  int moveto;
  int newloc;
  vector<string> shifted; // The words we shifted
  vector<char> alignment ; // for pra_more output
  vector<string> aftershift; // for pra_more output
  // This is used to store the cost of a shift, so we don't have to
  // calculate it multiple times.
  double cost;
  void set(terShift l_terShift);
  void set(terShift *l_terShift);
  void erase();
};

}
#endif
