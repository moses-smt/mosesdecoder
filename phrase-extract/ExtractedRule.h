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

#include "PhraseOrientation.h"

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
  std::string sourceContextLeft;
  std::string sourceContextRight;
  std::string targetContextLeft;
  std::string targetContextRight;
  std::string sourceHoleString;
  std::string targetHoleString;
  std::string targetSyntacticPreference;
  int startT;
  int endT;
  int startS;
  int endS;
  float count;
  double pcfgScore;
  PhraseOrientation::REO_CLASS l2rOrientation;
  PhraseOrientation::REO_CLASS r2lOrientation;

  ExtractedRule(int sT, int eT, int sS, int eS)
    : source()
    , target()
    , alignment()
    , alignmentInv()
    , sourceContextLeft()
    , sourceContextRight()
    , targetContextLeft()
    , targetContextRight()
    , sourceHoleString()
    , targetHoleString()
    , targetSyntacticPreference()
    , startT(sT)
    , endT(eT)
    , startS(sS)
    , endS(eS)
    , count(0)
    , pcfgScore(0.0)
    , l2rOrientation(PhraseOrientation::REO_CLASS_UNKNOWN)
    , r2lOrientation(PhraseOrientation::REO_CLASS_UNKNOWN)
  { }
};

}

#endif
