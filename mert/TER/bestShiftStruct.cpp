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
#include "bestShiftStruct.h"

using namespace std;

namespace TERCPPNS_TERCpp
{
bestShiftStruct::bestShiftStruct()
{
  m_best_shift=new terShift();
  m_best_align=new terAlignment();
  m_empty=new bool(false);
}
bestShiftStruct::~bestShiftStruct()
{
  delete(m_best_align);
  delete(m_best_shift);
}
void bestShiftStruct::setEmpty(bool b)
{
  m_empty=new bool(b);
}
void bestShiftStruct::setBestShift(terShift * l_terShift)
{
  m_best_shift->set(l_terShift);
}
void bestShiftStruct::setBestAlign(terAlignment * l_terAlignment)
{
  m_best_align->set(l_terAlignment);
}
string bestShiftStruct::toString()
{
  stringstream s;
  s << m_best_shift->toString() << endl;
  s << m_best_align->toString() << endl;
//	    s << (*m_empty) << endl;
  return s.str();
}
bool bestShiftStruct::getEmpty()
{
  return (*(m_empty));
}





}
