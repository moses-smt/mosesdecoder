// $Id$

/***********************************************************************
Moses - statistical machine translation system
Copyright (C) 2006-2011 University of Edinburgh

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

#include <cassert>
#include <limits>
#include <iostream>
#include <memory>
#include <sstream>

#include "FFState.h"
#include "LanguageModel.h"
#include "LanguageModelImplementation.h"
#include "TypeDef.h"
#include "Util.h"
#include "Manager.h"
#include "ChartManager.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{
LanguageModel::LanguageModel(ScoreIndexManager &scoreIndexManager, LanguageModelImplementation *implementation) :
  m_implementation(implementation)
{
  m_enableOOVFeature = StaticData::Instance().GetLMEnableOOVFeature(); 
  scoreIndexManager.AddScoreProducer(this);
#ifndef WITH_THREADS
  // ref counting handled by boost otherwise
  m_implementation->IncrementReferenceCount();
#endif
}

LanguageModel::LanguageModel(ScoreIndexManager &scoreIndexManager, LanguageModel *loadedLM) :
  m_implementation(loadedLM->m_implementation)
{
  m_enableOOVFeature = StaticData::Instance().GetLMEnableOOVFeature(); 
  scoreIndexManager.AddScoreProducer(this);
#ifndef WITH_THREADS
  // ref counting handled by boost otherwise
  m_implementation->IncrementReferenceCount();
#endif
}

LanguageModel::~LanguageModel()
{
#ifndef WITH_THREADS
  if(m_implementation->DecrementReferenceCount() == 0)
    delete m_implementation;
#endif
}

// don't inline virtual funcs...
size_t LanguageModel::GetNumScoreComponents() const
{
  if (m_enableOOVFeature) {
    return 2;
  } else {
    return 1;
  }
}

float LanguageModel::GetWeight() const
{
  size_t lmIndex = StaticData::Instance().GetScoreIndexManager().
                   GetBeginIndex(GetScoreBookkeepingID());
  return StaticData::Instance().GetAllWeights()[lmIndex];
}

float LanguageModel::GetOOVWeight() const
{
  if (!m_enableOOVFeature) return 0;
  size_t lmIndex = StaticData::Instance().GetScoreIndexManager().
                   GetBeginIndex(GetScoreBookkeepingID());
  return StaticData::Instance().GetAllWeights()[lmIndex+1];
  
}

const FFState* LanguageModel::EmptyHypothesisState(const InputType &/*input*/) const
{
  // This is actually correct.  The empty _hypothesis_ has <s> in it.  Phrases use m_emptyContextState.
  return m_implementation->NewState(m_implementation->GetBeginSentenceState());
}

}
