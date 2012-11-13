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

#include "Base.h"
#include "moses/TypeDef.h"
#include "moses/Util.h"
#include "moses/Manager.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/Phrase.h"
#include "moses/StaticData.h"
#include "util/exception.hh"

using namespace std;

namespace Moses {

LanguageModel::LanguageModel() : 
  StatefulFeatureFunction("LM", StaticData::Instance().GetLMEnableOOVFeature() ? 2 : 1 ) {
  m_enableOOVFeature = StaticData::Instance().GetLMEnableOOVFeature(); 
}


LanguageModel::~LanguageModel() {}

float LanguageModel::GetWeight() const {
  //return StaticData::Instance().GetAllWeights().GetScoresForProducer(this)[0];
  return StaticData::Instance().GetWeights(this)[0];
}

float LanguageModel::GetOOVWeight() const {
  if (m_enableOOVFeature) {
    //return StaticData::Instance().GetAllWeights().GetScoresForProducer(this)[1];
	return StaticData::Instance().GetWeights(this)[1];
  } else {
    return 0;
  }
}

void LanguageModel::IncrementalCallback(Incremental::Manager &manager) const {
  UTIL_THROW(util::Exception, "Incremental search is only supported by KenLM.");
}

} // namespace Moses
