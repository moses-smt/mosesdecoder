// $Id$

#include <iostream>
#include <sstream>

#include "ScoreProducer.h"

using namespace std;

namespace Moses
{

ScoreProducer::~ScoreProducer() {}

const vector<FName>& ScoreProducer::GetFeatureNames() const {
  if (m_names.size() != GetNumScoreComponents()) {
    const string& desc = GetScoreProducerDescription();
    if (GetNumScoreComponents() == 1) {
      m_names.push_back(FName(desc));
    } else {
      for (size_t i = 1; i <= GetNumScoreComponents(); ++i) {
        ostringstream id;
        id << i;
        m_names.push_back(FName(desc,id.str()));
      }
    }
  }
  return m_names;
}

}

