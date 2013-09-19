// $Id: SingleFactor.cpp,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $

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

#include "LM/SingleFactor.h"
#include "TypeDef.h"
#include "Util.h"
#include "FactorCollection.h"
#include "FFState.h"
#include "Phrase.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

LanguageModelSingleFactor::~LanguageModelSingleFactor() {}


std::string LanguageModelSingleFactor::GetScoreProducerDescription(unsigned) const
{
  std::ostringstream oss;
  // what about LMs that are over multiple factors at once, POS + stem, for example?
  oss << "LM_" << GetNGramOrder() << "gram";
  return oss.str();
}

struct PointerState : public FFState {
  const void* lmstate;
  PointerState(const void* lms) {
    lmstate = lms;
  }
  int Compare(const FFState& o) const {
    const PointerState& other = static_cast<const PointerState&>(o);
    if (other.lmstate > lmstate) return 1;
    else if (other.lmstate < lmstate) return -1;
    return 0;
  }
};

LanguageModelPointerState::LanguageModelPointerState()
{
  m_nullContextState = new PointerState(NULL);
  m_beginSentenceState = new PointerState(NULL);
}

LanguageModelPointerState::~LanguageModelPointerState() {}

const FFState *LanguageModelPointerState::GetNullContextState() const
{
  return m_nullContextState;
}

const FFState *LanguageModelPointerState::GetBeginSentenceState() const
{
  return m_beginSentenceState;
}

FFState *LanguageModelPointerState::NewState(const FFState *from) const
{
  //std::cout << "SINGLE FACTOR : MAKING NEW STATE" << std::endl;
  return new PointerState(from ? static_cast<const PointerState*>(from)->lmstate : NULL);
}

LMResult LanguageModelPointerState::GetValueForgotState(const std::vector<const Word*> &contextFactor, FFState &outState) const
{
   //std::cout << "SINGLE FACTOR" << std::endl;
    //std::cout << "DISPLAYING CONTEXT WORD : " <<  contextFactor.size() << std::endl;
   /*std::vector<const Word*>::const_iterator itr_context;
   for(itr_context = contextFactor.begin(); itr_context!=contextFactor.end();itr_context++)
                            {
                                std::cout << "CONTEXT FACTOR : " << **itr_context << std::endl;
                            }

  std::cout << "SINGLE FACTOR : get value for got state" << std::endl;*/
  return GetValue(contextFactor, &static_cast<PointerState&>(outState).lmstate);
}

}




