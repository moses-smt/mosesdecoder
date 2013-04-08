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

#ifndef moses_LanguageModelBackward_h
#define moses_LanguageModelBackward_h

#include <string>

#include "moses/LM/Ken.h"

namespace Moses {

//! This will also load. Returns a templated KenLM class
LanguageModel *ConstructBackwardLM(const std::string &file, FactorType factorType, bool lazy);

/*
 * An implementation of single factor backward LM using Kenneth's code.
 */
template <class Model> class BackwardLanguageModel : public LanguageModelKen<Model> {
  public:
    BackwardLanguageModel(const std::string &file, FactorType factorType, bool lazy);

    virtual const FFState *EmptyHypothesisState(const InputType &/*input*/) const;

  private:
 
    // This line is required to make the parent class's protected member visible to this class
    using LanguageModelKen<Model>::m_ngram;
   
};

} // namespace Moses

#endif

// To create a sample backward language model using SRILM:
// 
// (ngram-count and reverse-text are SRILM programs)
//
// head -n 49 ./contrib/synlm/hhmm/LICENSE | tail -n 45 | tr '\n' ' ' | ./scripts/ems/support/split-sentences.perl | ./scripts/tokenizer/lowercase.perl | ./scripts/tokenizer/tokenizer.perl | reverse-text | ngram-count -order 3 -text - -lm - > lm/backward.arpa
