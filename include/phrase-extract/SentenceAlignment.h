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
#ifndef SENTENCE_ALIGNMENT_H_INCLUDED_
#define SENTENCE_ALIGNMENT_H_INCLUDED_

#include <string>
#include <vector>

namespace MosesTraining
{

class SentenceAlignment
{
public:
  std::vector<std::string> target;
  std::vector<std::string> source;
  std::vector<int> alignedCountS;
  std::vector<std::vector<int> > alignedToT, alignedToS;
  int sentenceID;
  std::string weightString;

  virtual ~SentenceAlignment();

  virtual bool processTargetSentence(const char *, int, bool boundaryRules);

  virtual bool processSourceSentence(const char *, int, bool boundaryRules);

  bool create(char targetString[], char sourceString[],
              char alignmentString[], char weightString[], int sentenceID, bool boundaryRules);

  void invertAlignment();

};

}


#endif
