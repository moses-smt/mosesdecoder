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
#include "stringHasher.h"
// The following class defines a hash function for strings


using namespace std;

namespace HashMapSpace
{
stringHasher::stringHasher ( long cle, string cleTxt, string valueTxt )
{
  m_hashKey=cle;
  m_key=cleTxt;
  m_value=valueTxt;
}
//     stringHasher::~stringHasher(){};*/
long  stringHasher::getHashKey()
{
  return m_hashKey;
}
string  stringHasher::getKey()
{
  return m_key;
}
string  stringHasher::getValue()
{
  return m_value;
}
void stringHasher::setValue ( string  value )
{
  m_value=value;
}


// typedef stdext::hash_map<string, string, stringhasher> HASH_S_S;
}
