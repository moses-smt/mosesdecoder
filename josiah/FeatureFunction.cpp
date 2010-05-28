/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2009 University of Edinburgh

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

#include <fstream>
#include "FeatureFunction.h"

namespace Josiah {

FeatureFunction::~FeatureFunction(){} // n.b. is pure virtual, must be empty





FeatureFunctionScoreProducer::FeatureFunctionScoreProducer(const std::string & name, size_t numValues)
        :m_name(name), m_numValues(numValues) {
  StaticData& staticData = const_cast<StaticData&>(StaticData::Instance());
  const_cast<ScoreIndexManager&>(staticData.GetScoreIndexManager()).AddScoreProducer(this);
  vector<float> w(m_numValues); //default weight of 0
  staticData.SetWeightsForScoreProducer(this,w);
}



size_t FeatureFunctionScoreProducer::GetNumScoreComponents() const {
  return m_numValues;
}

string FeatureFunctionScoreProducer::GetScoreProducerDescription() const {
  return m_name;
} 

bool FeatureFunction::isConsistent(const ScoreComponentCollection& featureValues){
  ScoreComponentCollection expectedFeatureValues;
  assignScore(expectedFeatureValues);
  vector<float> expectedVector = expectedFeatureValues.GetScoresForProducer(&getScoreProducer());
  vector<float> actualVector = featureValues.GetScoresForProducer(&getScoreProducer());
  VERBOSE(2, "Checking Feature " << getScoreProducer().GetScoreProducerDescription() << endl);
  IFVERBOSE(2) {
    VERBOSE(2, "Expected: ");
    for (size_t i = 0; i < expectedVector.size(); ++i) {
      VERBOSE(2, expectedVector[i] << ",");
    }
    VERBOSE(2,endl);
    VERBOSE(2, "Actual: ");
    for (size_t i = 0; i < actualVector.size(); ++i) {
      VERBOSE(2, actualVector[i] << ",");
    }
    VERBOSE(2,endl);
  }
  
  if (expectedVector.size() != actualVector.size())  {
    VERBOSE(1, "FF Mismatch: Feature vectors were of different size: "<< getScoreProducer().GetScoreProducerDescription() << endl);
    return false;
  }
  for (size_t i = 0; i < expectedVector.size(); ++i) {
    if (expectedVector[i] != actualVector[i]) {
      VERBOSE(1, "FF Mismatch: Expected[" << i << "] = " << expectedVector[i] << " Actual[" << i << "] = " << actualVector[i] << 
        " " << getScoreProducer().GetScoreProducerDescription() << endl);
      return false;
    }
  }
  return true;
}

 
}//namespace
