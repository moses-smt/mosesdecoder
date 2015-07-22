// modified for mbot support
/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#pragma once
#ifndef RULEEXISTMBOT_H_INCLUDED_
#define RULEEXISTMBOT_H_INCLUDED_

#include <vector>

#include "HoleMBOT.h"


// repository of extracted phrase pairs
// which are potential holes in larger phrase pairs
class RuleExistMBOT
{
protected:
  std::vector< std::vector<HoleList> > m_phraseExist;
  // indexed by source position and source length
  // maps to list of holes where <int, int> are target pos

public:
  RuleExistMBOT(size_t size)
    :m_phraseExist(size) {
    // size is the length of the source sentence
    for (size_t pos = 0; pos < size; ++pos) {
      // create empty hole lists
      std::vector<HoleList> &endVec = m_phraseExist[pos];
      endVec.resize(size - pos);
    }
  }

  void Add(int startS, int endS, std::vector<std::pair<int,int> > spanT) {
	//std::cout << "RuleExistMBOT::Add" << std::endl;
    m_phraseExist[startS][endS - startS].push_back(HoleMBOT(startS, endS, spanT));
  }

  const HoleList &GetSourceHoles(int startS, int endS) const {
    const HoleList &sourceHoles = m_phraseExist[startS][endS - startS];
    return sourceHoles;
  }

};


#endif
