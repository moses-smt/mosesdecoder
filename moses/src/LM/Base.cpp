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

#include "LM/Base.h"
#include "TypeDef.h"
#include "Util.h"
#include "Manager.h"
#include "ChartManager.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"

using namespace std;

namespace Moses {

LanguageModel::LanguageModel() {
  m_enableOOVFeature = StaticData::Instance().GetLMEnableOOVFeature(); 
}

void LanguageModel::Init(ScoreIndexManager &scoreIndexManager) {
  scoreIndexManager.AddScoreProducer(this);
}

LanguageModel::~LanguageModel() {}

// don't inline virtual funcs...
size_t LanguageModel::GetNumScoreComponents() const {
  if (m_enableOOVFeature) {
    return 2;
  } else {
    return 1;
  }
}

float LanguageModel::GetWeight() const {
  size_t lmIndex = StaticData::Instance().GetScoreIndexManager().
                   GetBeginIndex(GetScoreBookkeepingID());
  return StaticData::Instance().GetAllWeights()[lmIndex];
}

float LanguageModel::GetOOVWeight() const {
  if (!m_enableOOVFeature) return 0;
  size_t lmIndex = StaticData::Instance().GetScoreIndexManager().
                   GetBeginIndex(GetScoreBookkeepingID());
  return StaticData::Instance().GetAllWeights()[lmIndex+1];
}

} // namespace Moses
