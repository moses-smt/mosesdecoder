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

#include "Derivation.h"

using namespace std;
using namespace Moses;

namespace Josiah {

  bool Derivation::PhraseAlignment::operator<(const PhraseAlignment& other) const {
  if (_sourceSegment < other._sourceSegment) return true;
  if (other._sourceSegment < _sourceSegment) return false;
  return _target < other._target;
  }

  Derivation::Derivation(const Sample& sample) {
   
    m_featureValues = sample.GetFeatureValues();
  
    const Hypothesis* currHypo = sample.GetTargetTail();
    while ((currHypo = (currHypo->GetNextHypo()))) {
      TargetPhrase targetPhrase = currHypo->GetTargetPhrase();
      m_alignments.push_back(
        PhraseAlignment(currHypo->GetCurrSourceWordsRange(), Phrase(targetPhrase)));
    }
    
    const vector<float> & weights = StaticData::Instance().GetAllWeights();
    m_score = m_featureValues.InnerProduct(weights);
  }
  

  //FIXME: This may not be the most efficient way of mapping derivations, but will do for now
  bool Derivation::operator <(const Derivation & other) const {
    bool result = m_alignments < other.m_alignments;
    return result;
  }

  ostream& operator<<(ostream& out, const Derivation& d) {
    //FIXME: Just print the target sentence and feature values for now
    out << "Target: <<";
    for (size_t i = 0; i < d.m_alignments.size(); ++i) {
      out << d.m_alignments[i]._target << " ";
    }
    out << ">> Feature values: ";
    out << d.m_featureValues;
    out << " Score: ";
    out << d.m_score;
    return out;
  }
  
  
  void DerivationCollector::collect(Sample& sample) {
    VERBOSE(1,"Collected: " << *sample.GetSampleHypothesis() << endl);
    ++m_counts[Derivation(sample)];
    ++m_n;
  }
  
  void DerivationCollector::getTopN(size_t n, vector<DerivationProbability>& derivations) {
    derivations.clear();
    for (map<Derivation,size_t>::iterator i = m_counts.begin(); i != m_counts.end(); ++i) {
      float probability = (float)i->second/m_n;
      derivations.push_back(DerivationProbability(&(i->first),probability));
    }
    DerivationProbGreaterThan comparator;
    /*for (size_t i = 0; i < derivations.size(); ++i) {
      const Derivation* d = derivations[i].first;
      float probability = derivations[i].second;
      cout << *d << endl;
      cout << probability << endl;
  }*/
    sort(derivations.begin(),derivations.end(),comparator);
    while (derivations.size() > n) {
      derivations.pop_back();
    }
    //cout << derivations.size() << endl;
  }

}//namespace
