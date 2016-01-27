#include <vector>
#include <limits>
#include <boost/math/special_functions/fpclassify.hpp>
#include <assert.h>
#include "TargetPreferencesFeature.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/TreeInput.h"
#include "moses/PP/TargetPreferencesPhraseProperty.h"


using namespace std;

namespace Moses
{

void TargetPreferencesFeatureState::AddProbabilityForLHSLabel(size_t label, double cost)
{
  std::pair< std::map<size_t,double>::iterator, bool > inserted =
    m_probabilitiesForLHSLabels.insert(std::pair<size_t,double>(label,cost));
  if ( !inserted.second ) {
    (inserted.first)->second += cost;
  }
}

void TargetPreferencesFeatureState::NormalizeProbabilitiesForLHSLabels(double denominator)
{
  for ( std::map<size_t,double>::iterator iter=m_probabilitiesForLHSLabels.begin();
        iter!=m_probabilitiesForLHSLabels.end(); ++iter ) {
    (iter->second) /= denominator;
  }
}

double TargetPreferencesFeatureState::GetProbabilityForLHSLabel(size_t label, bool &isMatch) const
{
  std::map<size_t,double>::const_iterator iter = m_probabilitiesForLHSLabels.find(label);
  if ( iter != m_probabilitiesForLHSLabels.end() ) {
    isMatch = true;
    return iter->second;
  }
  isMatch = false;
  return 0;
}

size_t TargetPreferencesFeatureState::hash() const
{
  if (!m_distinguishStates) {
    return 0;
  }
  size_t ret = 0;
  boost::hash_combine(ret, m_probabilitiesForLHSLabels.size());
  for (std::map<size_t,double>::const_iterator it=m_probabilitiesForLHSLabels.begin();
       it!=m_probabilitiesForLHSLabels.end(); ++it) {
    boost::hash_combine(ret, it->first);
  }
  return ret;
};

bool TargetPreferencesFeatureState::operator==(const FFState& other) const
{
  if (!m_distinguishStates) {
    return true;
  }

  if (this == &other) {
    return true;
  }

  const TargetPreferencesFeatureState* otherState =
    dynamic_cast<const TargetPreferencesFeatureState*>(&other);
  UTIL_THROW_IF2(otherState == NULL, "Wrong state type");

  if (m_probabilitiesForLHSLabels.size() != (otherState->m_probabilitiesForLHSLabels).size()) {
    return  false;
  }
  std::map<size_t,double>::const_iterator thisIt, otherIt;
  for (thisIt=m_probabilitiesForLHSLabels.begin(), otherIt=(otherState->m_probabilitiesForLHSLabels).begin();
       thisIt!=m_probabilitiesForLHSLabels.end(); ++thisIt, ++otherIt) {
    if (thisIt->first != otherIt->first) {
      return false;
    }
  }
  return true;
};


TargetPreferencesFeature::TargetPreferencesFeature(const std::string &line)
  : StatefulFeatureFunction(2, line)
  , m_featureVariant(0)
  , m_distinguishStates(false)
  , m_noMismatches(false)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  VERBOSE(1, " Done." << std::endl);
  VERBOSE(1, " Feature variant: " << m_featureVariant << "." << std::endl);
}

TargetPreferencesFeature::~TargetPreferencesFeature()
{}

void TargetPreferencesFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "label-set-file") {
    m_labelSetFile = value;
  } else if (key == "unknown-word-labels-file") {
    m_unknownLeftHandSideFile = value;
  } else if (key == "variant") {
    m_featureVariant = Scan<size_t>(value);
  } else if (key == "distinguish-states") {
    m_distinguishStates = Scan<bool>(value);
  } else if (key == "no-mismatches") {
    m_noMismatches = Scan<bool>(value);
  } else {
    StatefulFeatureFunction::SetParameter(key, value);
  }
}


void TargetPreferencesFeature::Load(AllOptions::ptr const& opts)
{
  // don't change the loading order!
  LoadLabelSet();
  LoadUnknownLeftHandSideFile();
}

void TargetPreferencesFeature::LoadLabelSet()
{
  FEATUREVERBOSE(2, "Loading label set from file " << m_labelSetFile << " ...");
  InputFileStream inFile(m_labelSetFile);

  // read label set
  std::string line;
  m_labels.clear();
  m_labelsByIndex.clear();
  while (getline(inFile, line)) {
    std::istringstream tokenizer(line);
    std::string label;
    size_t index;
    try {
      tokenizer >> label >> index;
    } catch (const std::exception &e) {
      UTIL_THROW2(GetScoreProducerDescription()
                  << ": Error reading label set file " << m_labelSetFile << " .");
    }
    std::pair< boost::unordered_map<std::string,size_t>::iterator, bool > inserted = m_labels.insert( std::pair<std::string,size_t>(label,index) );
    UTIL_THROW_IF2(!inserted.second, GetScoreProducerDescription()
                   << ": Label set file " << m_labelSetFile << " should contain each label only once.");

    if (index >= m_labelsByIndex.size()) {
      m_labelsByIndex.resize(index+1);
    }
    m_labelsByIndex[index] = label;
  }

  inFile.Close();

  std::list<std::string> specialLabels;
  specialLabels.push_back("GlueTop");
  for (std::list<std::string>::const_iterator iter=specialLabels.begin();
       iter!=specialLabels.end(); ++iter) {
    boost::unordered_map<std::string,size_t>::iterator found = m_labels.find(*iter);
    UTIL_THROW_IF2(found == m_labels.end(), GetScoreProducerDescription()
                   << ": Label set file " << m_labelSetFile << " should contain an entry for the special label \"" << *iter << "\".");
    if (!(found->first).compare("GlueTop")) {
      m_GlueTopLabel = found->second;
    }
  }
  FEATUREVERBOSE2(2, " Done." << std::endl);
}

// Make sure to call this method _after_ LoadLabelSet()
void TargetPreferencesFeature::LoadUnknownLeftHandSideFile()
{
  FEATUREVERBOSE(2, "Loading left-hand side labels for unknowns from file " << m_unknownLeftHandSideFile << std::endl);
  InputFileStream inFile(m_unknownLeftHandSideFile);

  // read left-hand side labels for unknowns
  std::string line;
  m_unknownLHSProbabilities.clear();
  double countsSum = 0.0;
  while (getline(inFile, line)) {
    istringstream tokenizer(line);
    std::string label;
    double count;
    tokenizer >> label;
    tokenizer >> count;
    boost::unordered_map<std::string,size_t>::iterator found = m_labels.find( label );
    if ( found != m_labels.end() ) {
      std::pair< std::map<size_t,double>::iterator, bool > inserted =
        m_unknownLHSProbabilities.insert( std::pair<size_t,double>(found->second,count) );
      if ( !inserted.second ) {
        (inserted.first)->second += count;
      }
      countsSum += count;
    } else {
      FEATUREVERBOSE(1, "WARNING: undefined label \"" << label << "\" in file " << m_unknownLeftHandSideFile << std::endl);
    }
  }
  // compute probabilities from counts
  countsSum += (double)m_labels.size();
  for (std::map<size_t,double>::iterator iter=m_unknownLHSProbabilities.begin();
       iter!=m_unknownLHSProbabilities.end(); ++iter) {
    iter->second /= countsSum;
  }

  IFFEATUREVERBOSE(3) {
    for (std::map<size_t,double>::iterator iter=m_unknownLHSProbabilities.begin();
         iter!=m_unknownLHSProbabilities.end(); ++iter) {
      FEATUREVERBOSE(3, GetScoreProducerDescription() << "::LoadUnknownLeftHandSideFile(): " << iter->first << " " << iter->second << std::endl);
    }
  }

  inFile.Close();
}

FFState* TargetPreferencesFeature::EvaluateWhenApplied(
  const ChartHypothesis& hypo,
  int featureID, // used to index the state in the previous hypotheses
  ScoreComponentCollection* accumulator) const
{
  streamsize cerr_precision =  std::cerr.precision();
  std::cerr.precision(20); // TODO: remove. just for debug purposes.

  // dense scores
  std::vector<float> newScores(m_numScoreComponents,0); // m_numScoreComponents == 2

  // state: used to store tree probabilities of partial hypotheses
  // and access the respective tree probabilities of subderivations
  TargetPreferencesFeatureState *state = new TargetPreferencesFeatureState(m_distinguishStates);

  size_t nNTs = 1;
  double overallTreeProbability = 0.0;
  bool isGlueGrammarRule = false;

  // read TargetPreferences property
  const TargetPhrase &currTarPhr = hypo.GetCurrTargetPhrase();

  FEATUREVERBOSE(2, "Phrase: " << currTarPhr << std::endl);

  if (const PhraseProperty *property = currTarPhr.GetProperty("TargetPreferences")) {

    const TargetPreferencesPhraseProperty *targetPreferencesPhraseProperty = static_cast<const TargetPreferencesPhraseProperty*>(property);

//    IFFEATUREVERBOSE(2) {
//      const std::string *targetPreferencesPhrasePropertyValueString = targetPreferencesPhraseProperty->GetValueString();
//      if (targetPreferencesPhrasePropertyValueString) {
//        FEATUREVERBOSE(2, "PreferencesPhraseProperty " << *targetPreferencesPhrasePropertyValueString << std::endl);
//      } else {
//        FEATUREVERBOSE(2, "PreferencesPhraseProperty NULL" << std::endl);
//      }
//    }

    nNTs = targetPreferencesPhraseProperty->GetNumberOfNonTerminals();
    double totalCount = targetPreferencesPhraseProperty->GetTotalCount();

    // get index map for underlying hypotheses
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      currTarPhr.GetAlignNonTerm().GetNonTermIndexMap();

    // retrieve states from previous hypotheses, if any
    std::vector< const TargetPreferencesFeatureState* > prevStatesByNonTerminal(nNTs-1);

    if (nNTs > 1) { // rule has right-hand side non-terminals, i.e. it's a hierarchical rule
      size_t nonTerminalNumber = 0;

      for (size_t phrasePos=0; phrasePos<currTarPhr.GetSize(); ++phrasePos) {
        // consult rule for either word or non-terminal
        const Word &word = currTarPhr.GetWord(phrasePos);
        if ( word.IsNonTerminal() ) {
          // non-terminal: consult subderivation
          size_t nonTermIndex = nonTermIndexMap[phrasePos];
          const ChartHypothesis *prevHypo = hypo.GetPrevHypo(nonTermIndex);
          const TargetPreferencesFeatureState* prevState =
            static_cast<const TargetPreferencesFeatureState*>(prevHypo->GetFFState(featureID));
          prevStatesByNonTerminal[nonTerminalNumber] = prevState;

          IFFEATUREVERBOSE(2) {
            // some log output that is not required in any way for the functionality
            const std::map<size_t,double> &prevHypoTreeProbabilities =
              prevStatesByNonTerminal[nonTerminalNumber]->GetProbabilitiesForLHSLabels();
            FEATUREVERBOSE(2, "Previous tree probs:");
            for (std::map<size_t,double>::const_iterator iter=prevHypoTreeProbabilities.begin();
                 iter!=prevHypoTreeProbabilities.end(); ++iter) {
              FEATUREVERBOSE2(2, " " << m_labelsByIndex[iter->first] << " " << iter->second);
            }
            FEATUREVERBOSE2(2, std::endl);
          }

          ++nonTerminalNumber;
        }
      }
    }

    // inspect labelled rule items

    overallTreeProbability = 0.0;

    const std::list<TargetPreferencesPhrasePropertyItem> &targetPreferencesItems = targetPreferencesPhraseProperty->GetTargetPreferencesItems();

    for (std::list<TargetPreferencesPhrasePropertyItem>::const_iterator targetPreferencesItem = targetPreferencesItems.begin();
         targetPreferencesItem != targetPreferencesItems.end(); ++targetPreferencesItem) {

      const std::list<size_t> &targetPreferencesRHS = targetPreferencesItem->GetTargetPreferencesRHS();
      const std::list< std::pair<size_t,float> > &targetPreferencesLHSList = targetPreferencesItem->GetTargetPreferencesLHSList();

      assert(targetPreferencesRHS.size() == nNTs-1);

      size_t currentTargetLabelsMismatches = nNTs - 1;
      double matchingLabelsProbabilityProduct = 1.0;

      size_t nonTerminalNumber=0;
      for (std::list<size_t>::const_iterator targetPreferencesRHSIt = targetPreferencesRHS.begin();
           targetPreferencesRHSIt != targetPreferencesRHS.end(); ++targetPreferencesRHSIt, ++nonTerminalNumber) {

        bool isLabelMatch = false;
        double matchingLabelsProbability =
          prevStatesByNonTerminal[nonTerminalNumber]->GetProbabilityForLHSLabel(*targetPreferencesRHSIt,
              isLabelMatch);
        matchingLabelsProbabilityProduct *= matchingLabelsProbability;

        if ( isLabelMatch ) {
          currentTargetLabelsMismatches -= 1;
        }
      }

      FEATUREVERBOSE(2, "matchingLabelsProbabilityProduct = " << matchingLabelsProbabilityProduct << std::endl);

      // LHS labels seen with this RHS
      for (std::list< std::pair<size_t,float> >::const_iterator targetPreferencesLHSIt = targetPreferencesLHSList.begin();
           targetPreferencesLHSIt != targetPreferencesLHSList.end(); ++targetPreferencesLHSIt) {

        size_t targetPreferenceLHS = targetPreferencesLHSIt->first;

        if ( targetPreferenceLHS == m_GlueTopLabel ) {
          isGlueGrammarRule = true;
        }

        // proceed with the actual probability computations
        double ruleTargetPreferenceCount = targetPreferencesLHSIt->second;
        double ruleTargetPreferenceProbability = ruleTargetPreferenceCount / totalCount;

        FEATUREVERBOSE(2, "  ruleTargetPreferenceProbability = " << ruleTargetPreferenceProbability << std::endl);

        double weightedTargetPreferenceRuleProbability = ruleTargetPreferenceProbability * matchingLabelsProbabilityProduct;
        if ( weightedTargetPreferenceRuleProbability != 0 ) {
          state->AddProbabilityForLHSLabel(targetPreferenceLHS, weightedTargetPreferenceRuleProbability);
        }
        overallTreeProbability += weightedTargetPreferenceRuleProbability;
      }
    }

    IFFEATUREVERBOSE(2) {
      FEATUREVERBOSE(2, "overallTreeProbability = " << overallTreeProbability);
      if ( overallTreeProbability > 1.0001 ) { // account for some rounding error
        FEATUREVERBOSE2(2, " -- WARNING: overallTreeProbability > 1");
      }
      FEATUREVERBOSE2(2, std::endl);
    }

    if ( overallTreeProbability != 0 ) {
      UTIL_THROW_IF2(!boost::math::isnormal(overallTreeProbability), GetScoreProducerDescription()
                     << ": Oops. Numerical precision issues.");
      state->NormalizeProbabilitiesForLHSLabels(overallTreeProbability);
    }

  } else {

    // abort with error message if the phrase does not translate an unknown word
    UTIL_THROW_IF2(!currTarPhr.GetWord(0).IsOOV(), GetScoreProducerDescription()
                   << ": Missing TargetPreferences property. Please check phrase table and glue rules.");

    // unknown word
    overallTreeProbability = 1.0;

    for (std::map<size_t,double>::const_iterator iter=m_unknownLHSProbabilities.begin();
         iter!=m_unknownLHSProbabilities.end(); ++iter) {
      // update state
      state->AddProbabilityForLHSLabel(iter->first, iter->second);
    }
  }

  FEATUREVERBOSE(2, "-> OVERALLTREEPROB  = " << overallTreeProbability << std::endl);

  // add scores

  // tree probability (preference grammar style)
  newScores[0] = (overallTreeProbability == 0 ? 0 : std::log(overallTreeProbability) );
  if ( m_noMismatches && (overallTreeProbability == 0) && !isGlueGrammarRule ) {
    newScores[0] = -std::numeric_limits<float>::infinity();
  }
  // tree mismatch penalty
  // TODO: deactivate the tree mismatch penalty score component automatically if feature configuration parameter no-mismatches=true
  newScores[1] = (overallTreeProbability == 0 ? 1 : 0 );

  accumulator->PlusEquals(this, newScores);

  std::cerr.precision(cerr_precision);
  return state;
}

}

