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
#include "Gibbler.h"
#include "DummyScoreProducers.h"
#include "WeightManager.h"

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
    
    const FVector& weights = WeightManager::instance().get();
    m_score = inner_product(m_featureValues, weights);
  }
  

  //FIXME: This may not be the most efficient way of mapping derivations, but will do for now
  bool Derivation::operator <(const Derivation & other) const {
    bool result = m_alignments < other.m_alignments;
    return result;
  }
  
  void Derivation::getTargetFactors(std::vector<const Factor*>& sentence) const {
    for (vector<PhraseAlignment>::const_iterator i = m_alignments.begin(); i != m_alignments.end(); ++i) {
      const Phrase& targetPhrase = i->_target;
      for (size_t j = 0; j < targetPhrase.GetSize(); ++j) {
        sentence.push_back(targetPhrase.GetFactor(j,0));
      }
    }
  }
  
  int Derivation::getTargetSentenceSize() const { //shortcut, extract tgt size from feature vector
      std::vector<std::string> words;
      getTargetSentence(words);
      return words.size();
  }

  void Derivation::getTargetSentence(std::vector<std::string>& targetWords ) const {
    for (vector<PhraseAlignment>::const_iterator i = m_alignments.begin(); i != m_alignments.end(); ++i) {
      const Phrase& targetPhrase = i->_target;
      for (size_t j = 0; j < targetPhrase.GetSize(); ++j) {
        targetWords.push_back(targetPhrase.GetWord(j).GetFactor(0)->GetString());
      }
    }
  }
  
  ostream& operator<<(ostream& out, const Derivation& d) {
    out << "Target: << ";
    for (size_t i = 0; i < d.m_alignments.size(); ++i) {
      out << d.m_alignments[i]._target;
      out << d.m_alignments[i]._sourceSegment << " ";
    }
    out << ">> Feature values: ";
    out << d.m_featureValues;
    out << " Score: ";
    out << d.m_score;
    return out;
  }
  
}
//namespace
