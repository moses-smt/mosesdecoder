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

#include <limits>
#include <iostream>
#include <sstream>

#include "SingleFactor.h"
#include "PointerState.h"
#include "moses/FF/FFState.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/StaticData.h"
#include "moses/FactorTypeSet.h"

using namespace std;

namespace Moses
{

LanguageModelSingleFactor::LanguageModelSingleFactor(const std::string &line)
  :LanguageModelImplementation(line)
{
  m_nullContextState = new PointerState(NULL);
  m_beginSentenceState = new PointerState(NULL);
}

LanguageModelSingleFactor::~LanguageModelSingleFactor() {}

const FFState *LanguageModelSingleFactor::GetNullContextState() const
{
  return m_nullContextState;
}

const FFState *LanguageModelSingleFactor::GetBeginSentenceState() const
{
  return m_beginSentenceState;
}

FFState *LanguageModelSingleFactor::NewState(const FFState *from) const
{
  return new PointerState(from ? static_cast<const PointerState*>(from)->lmstate : NULL);
}

LMResult LanguageModelSingleFactor::GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState) const
{
  return GetValue(contextFactor, &static_cast<PointerState&>(outState).lmstate);
}

bool LanguageModelSingleFactor::IsUseable(const FactorMask &mask) const
{
  bool ret = mask[m_factorType];
  return ret;
}

void LanguageModelSingleFactor::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "factor") {
    m_factorType = Scan<FactorType>(value);
  } else {
    LanguageModelImplementation::SetParameter(key, value);
  }
}

}




