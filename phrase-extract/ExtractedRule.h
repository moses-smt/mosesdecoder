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
#ifndef EXTRACTEDRULE_H_INCLUDED_
#define EXTRACTEDRULE_H_INCLUDED_

#include <string>
#include <iostream>
#include <sstream>
#include <map>

namespace MosesTraining
{

// sentence-level collection of rules
class ExtractedRule
{
public:
  std::string source;
  std::string target;
  std::string alignment;
  std::string alignmentInv;
  std::string orientation;
  std::string orientationForward;
  std::string sourceContextLeft;
  std::string sourceContextRight;
  std::string targetContextLeft;
  std::string targetContextRight;
  std::string sourceHoleString;
  std::string targetHoleString;
  int startT;
  int endT;
  int startS;
  int endS;
  float count;
  double pcfgScore;

  ExtractedRule(int sT, int eT, int sS, int eS)
    : source()
    , target()
    , alignment()
    , alignmentInv()
    , orientation()
    , orientationForward()
    , sourceContextLeft()
    , sourceContextRight()
    , targetContextLeft()
    , targetContextRight()
    , sourceHoleString()
    , targetHoleString()
    , startT(sT)
    , endT(eT)
    , startS(sS)
    , endS(eS)
    , count(0)
    , pcfgScore(0.0) {
  }
};

}

#endif
