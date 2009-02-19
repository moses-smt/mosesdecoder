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

#include "Score.h"


using namespace std;
using namespace Moses;


namespace Josiah {

  
  
float NamedValues::s_default = 0;
  
const float& NamedValues::operator[](const string& name) const {
  map<string,float>::const_iterator i = m_namedValues.find(name);
  if (i == m_namedValues.end()) {
    return s_default;
  } else {
    return i->second;
  }
}

float& NamedValues::operator[](const string& name) {
  return m_namedValues[name];
}



vector<float> WeightVector::getAllWeights() const {
  vector<float> weights(getMosesWeights());
  const vector<string>& featureNames = FeatureRegistry::instance()->getFeatureNames();
  for (size_t i = 0; i < featureNames.size(); ++i) {
    weights.push_back((*this)[featureNames[i]]);
  }
  return weights;
}

const vector< float > & WeightVector::getMosesWeights( ) const {
  return StaticData::Instance().GetAllWeights();
}


std::ostream& operator<<(std::ostream& out, const WeightVector& wv) {
  out << "<<";
  vector<float> weights = wv.getAllWeights();
  copy(weights.begin(),weights.end(),ostream_iterator<float>(out,","));
  out << ">>";
  return out;
}

vector< float > ScoreVector::getAllScores( ) const {
  vector<float> scores;
  for (size_t i = 0; i < m_mosesScores.size(); ++i) {
    scores.push_back(m_mosesScores[i]);
  }
  const vector<string>& featureNames = FeatureRegistry::instance()->getFeatureNames();
  for (size_t i = 0; i < featureNames.size(); ++i) {
    scores.push_back((*this)[featureNames[i]]);
  }
  return scores;
}



ScoreVector Josiah::ScoreVector::operator +( const ScoreVector& rhs ) const {
  ScoreVector sum;
  sum += *this;
  sum += rhs;
  return sum;
}

ScoreVector Josiah::ScoreVector::operator -( const ScoreVector& rhs ) const {
  ScoreVector diff;
  diff += *this;
  diff -= rhs;
  return diff;
}

void Josiah::ScoreVector::operator +=( const ScoreVector& rhs ) {
  m_mosesScores.PlusEquals(rhs.getMosesScores());
  const vector<string>& featureNames = FeatureRegistry::instance()->getFeatureNames();
  for (size_t i = 0; i < featureNames.size(); ++i) {
    (*this)[featureNames[i]] += rhs[featureNames[i]];
  }
}

void Josiah::ScoreVector::operator -=( const ScoreVector& rhs ) {
  m_mosesScores.MinusEquals(rhs.getMosesScores());
  const vector<string>& featureNames = FeatureRegistry::instance()->getFeatureNames();
  for (size_t i = 0; i < featureNames.size(); ++i) {
    (*this)[featureNames[i]] -= rhs[featureNames[i]];
  }
}

float Josiah::ScoreVector::operator*(const WeightVector& weights) const{
  float product = m_mosesScores.InnerProduct(weights.getMosesWeights());
  const vector<string>& featureNames = FeatureRegistry::instance()->getFeatureNames();
  for (size_t i = 0; i < featureNames.size(); ++i) {
    product += (*this)[featureNames[i]]*weights[featureNames[i]];
  }
  return product;
}

ostream& operator<<(ostream& out, const ScoreVector& sv){
  out << "<<";
  vector<float> scores = sv.getAllScores();
  copy(scores.begin(),scores.end(),ostream_iterator<float>(out,","));
  out << ">>";
  return out;
}

}





//namespace
