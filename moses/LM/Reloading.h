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

#ifndef moses_LanguageModelReloading_h
#define moses_LanguageModelReloading_h

#include <string>

#include "moses/LM/Ken.h"

#include "lm/state.hh"

namespace Moses
{

//! This will also load. Returns a templated reloading LM.
LanguageModel *ConstructReloadingLM(const std::string &line, const std::string &file, FactorType factorType, bool lazy);

class FFState;

/*
 * An implementation of single factor reloading LM using Kenneth's code.
 */
template <class Model> class ReloadingLanguageModel : public LanguageModelKen<Model>
{
public:
  ReloadingLanguageModel(const std::string &line, const std::string &file, FactorType factorType, bool lazy);

  virtual const FFState *EmptyHypothesisState(const InputType &/*input*/) const;

  virtual FFState *EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const;


private:

  // These lines are required to make the parent class's protected members visible to this class
  using LanguageModelKen<Model>::m_ngram;
  //  using LanguageModelKen<Model>::m_beginSentenceFactor;
  //using LanguageModelKen<Model>::m_factorType;
  //using LanguageModelKen<Model>::TranslateID;

};

} // namespace Moses

#endif

