/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2013- University of Edinburgh

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

#include <boost/lexical_cast.hpp>
#include <boost/unordered_set.hpp>

#include "util/exception.hh"
#include "util/tokenize_piece.hh"
#include "moses/TranslationModel/PhraseDictionaryInterpolated.h"

using namespace std;

namespace Moses
{

PhraseDictionaryInterpolated::PhraseDictionaryInterpolated
(size_t numScoreComponent,size_t numInputScores,const PhraseDictionaryFeature* feature):
  PhraseDictionary(numScoreComponent,feature),
  m_targetPhrases(NULL),
  m_languageModels(NULL) {}

bool PhraseDictionaryInterpolated::Load(
  const std::vector<FactorType> &input
  , const std::vector<FactorType> &output
  , const std::vector<std::string>& config
  , const std::vector<float> &weightT
  , size_t tableLimit
  , const LMList &languageModels
  , float weightWP)
{

  m_languageModels = &languageModels;
  m_weightT = weightT;
  m_tableLimit = tableLimit;
  m_weightWP = weightWP;

  //The config should be as follows:
  //0-3: type factor factor num-components (as usual)
  //4: combination mode (e.g. naive)
  //5-(length-2): List of phrase-table files
  //length-1: Weight string, in the same format as used for tmcombine

  UTIL_THROW_IF(config.size() < 7, util::Exception, "Missing fields from phrase table configuration: expected at least 7");
  UTIL_THROW_IF(config[4] != "naive", util::Exception, "Unsupported combination mode: '" << config[4] << "'");

  // Create the dictionaries
  for (size_t i = 5; i < config.size()-1; ++i) {
    m_dictionaries.push_back(DictionaryHandle(new PhraseDictionaryTreeAdaptor(
                               GetFeature()->GetNumScoreComponents(),
                               GetFeature()->GetNumInputScores(),
                               GetFeature())));
    bool ret = m_dictionaries.back()->Load(
                 input,
                 output,
                 config[i],
                 weightT,
                 0,
                 languageModels,
                 weightWP);
    if (!ret) return ret;
  }

  //Parse the weight strings
  for (util::TokenIter<util::SingleCharacter, false> featureWeights(config.back(), util::SingleCharacter(';')); featureWeights; ++featureWeights) {
    m_weights.push_back(vector<float>());
    float sum = 0;
    for (util::TokenIter<util::SingleCharacter, false> tableWeights(*featureWeights, util::SingleCharacter(',')); tableWeights; ++tableWeights) {
      const float weight = boost::lexical_cast<float>(*tableWeights);
      m_weights.back().push_back(weight);
      sum += weight;
    }
    UTIL_THROW_IF(m_weights.back().size() != m_dictionaries.size(), util::Exception,
                  "Number of weights (" << m_weights.back().size() <<
                  ") does not match number of dictionaries to combine (" << m_dictionaries.size() << ")");
    UTIL_THROW_IF(abs(sum - 1) > 0.01, util::Exception, "Weights not normalised");

  }

  //check number of weight sets. Make sure there is a weight for every score component
  //except for the last - which is assumed to be the phrase penalty.
  UTIL_THROW_IF(m_weights.size() != 1 && m_weights.size() != GetFeature()->GetNumScoreComponents()-1, util::Exception, "Unexpected number of weight sets");
  //if 1 weight set, then repeat
  if (m_weights.size() == 1) {
    while(m_weights.size() < GetFeature()->GetNumScoreComponents()-1) {
      m_weights.push_back(m_weights[0]);
    }
  }

  return true;
}

void PhraseDictionaryInterpolated::InitializeForInput(InputType const& source)
{
  for (size_t i = 0; i < m_dictionaries.size(); ++i) {
    m_dictionaries[i]->InitializeForInput(source);
  }
}

typedef
boost::unordered_set<TargetPhrase*,PhrasePtrHasher,PhrasePtrComparator> PhraseSet;


const TargetPhraseCollection*
PhraseDictionaryInterpolated::GetTargetPhraseCollection(const Phrase& src) const
{

  delete m_targetPhrases;
  m_targetPhrases = new TargetPhraseCollection();
  PhraseSet allPhrases;
  vector<PhraseSet> phrasesByTable(m_dictionaries.size());
  for (size_t i = 0; i < m_dictionaries.size(); ++i) {
    const TargetPhraseCollection* phrases = m_dictionaries[i]->GetTargetPhraseCollection(src);
    if (phrases) {
      for (TargetPhraseCollection::const_iterator j = phrases->begin();
           j != phrases->end(); ++j) {
        allPhrases.insert(*j);
        phrasesByTable[i].insert(*j);
      }
    }
  }
  ScoreComponentCollection sparseVector;
  for (PhraseSet::const_iterator i = allPhrases.begin(); i != allPhrases.end(); ++i) {
    TargetPhrase* combinedPhrase = new TargetPhrase((Phrase)**i);
    //combinedPhrase->ResetScore();
    //cerr << *combinedPhrase << " " << combinedPhrase->GetScoreBreakdown() << endl;
    combinedPhrase->SetSourcePhrase((*i)->GetSourcePhrase());
    combinedPhrase->SetAlignTerm(&((*i)->GetAlignTerm()));
    combinedPhrase->SetAlignNonTerm(&((*i)->GetAlignTerm()));
    Scores combinedScores(GetFeature()->GetNumScoreComponents());
    for (size_t j = 0; j < phrasesByTable.size(); ++j) {
      PhraseSet::const_iterator tablePhrase = phrasesByTable[j].find(combinedPhrase);
      if (tablePhrase != phrasesByTable[j].end()) {
        Scores tableScores = (*tablePhrase)->GetScoreBreakdown()
                             .GetScoresForProducer(GetFeature());
        //cerr << "Scores from " << j << " table: ";
        for (size_t k = 0; k < tableScores.size()-1; ++k) {
          //cerr << tableScores[k] << "(" << exp(tableScores[k]) << ") ";
          combinedScores[k] += m_weights[k][j] * exp(tableScores[k]);
          //cerr << m_weights[k][j] * exp(tableScores[k]) << " ";
        }
        //cerr << endl;
      }
    }
    //map back to log space
    //cerr << "Combined ";
    for (size_t k = 0; k < combinedScores.size()-1; ++k) {
      //cerr << combinedScores[k] << " ";
      combinedScores[k] = log(combinedScores[k]);
      //cerr << combinedScores[k] << " ";
    }
    //cerr << endl;
    combinedScores.back() = 1; //assume last is penalty
    combinedPhrase->SetScore(
      GetFeature(),
      combinedScores,
      sparseVector,
      m_weightT,
      m_weightWP,
      *m_languageModels);
    //cerr << *combinedPhrase << " " << combinedPhrase->GetScoreBreakdown() <<  endl;
    m_targetPhrases->Add(combinedPhrase);
  }

  m_targetPhrases->Prune(true,m_tableLimit);


  return m_targetPhrases;
}

}
