// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#ifndef moses_LanguageModelKen_h
#define moses_LanguageModelKen_h

#include <string>

#include "LanguageModelSingleFactor.h"
#include "kenlm/lm/left.hh"

namespace Moses
{

  // kenlm specific score value
  struct LMKenResult : public LMResult {
    unsigned char ngram_length;
  };

  // base-class for the actual LanguageModelKen; only here to provide a specific behaviour without exposing the implementation
  class LanguageModelKenBase : public LanguageModelSingleFactor {
    public:
      virtual ~LanguageModelKenBase();
      // scoring functions which provide more info than the common interface of LanguageModel
      virtual LMKenResult GetKenFullScoreGivenState(const std::vector<const Word*> &contextFactor, FFState &state) const = 0;
      virtual LMKenResult GetKenFullScoreForgotState(const std::vector<const Word*> &contextFactor, FFState &outState) const = 0;
};

class ScoreIndexManager;

// Doesn't actually load; moses wants the Load method for that.  It needs the file to autodetect binary format.
LanguageModelSingleFactor *ConstructKenLM(const std::string &file, bool lazy);

}


#endif
