#include <vector>
#include <limits>
#include <assert.h>
#include "SoftSourceSyntacticConstraintsFeature.h"
#include "moses/StaticData.h"
#include "moses/InputFileStream.h"
#include "moses/ScoreComponentCollection.h"
#include "moses/Hypothesis.h"
#include "moses/ChartHypothesis.h"
#include "moses/ChartManager.h"
#include "moses/FactorCollection.h"
#include "moses/TreeInput.h"
#include "moses/PP/SourceLabelsPhraseProperty.h"


using namespace std;

namespace Moses
{

SoftSourceSyntacticConstraintsFeature::SoftSourceSyntacticConstraintsFeature(const std::string &line)
  : StatelessFeatureFunction(3, line), m_featureVariant(0)
{
  VERBOSE(1, "Initializing feature " << GetScoreProducerDescription() << " ...");
  ReadParameters();
  VERBOSE(1, " Done.");
  VERBOSE(1, " Feature variant: " << m_featureVariant << "." << std::endl);
}

void SoftSourceSyntacticConstraintsFeature::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "sourceLabelSetFile") {
    m_sourceLabelSetFile = value;
  } else if (key == "coreSourceLabelSetFile") {
    m_coreSourceLabelSetFile = value;
  } else if (key == "targetSourceLeftHandSideJointCountFile") {
    m_targetSourceLHSJointCountFile = value;
  } else if (key == "featureVariant") {
    m_featureVariant = Scan<size_t>(value); // 0: only dense features, 1: no mismatches (also set weights 1 0 0 and tuneable=false), 2: with sparse features, 3: with sparse features for core labels only
  } else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}


void SoftSourceSyntacticConstraintsFeature::Load()
{
  // don't change the loading order!
  LoadSourceLabelSet();
  if (m_featureVariant == 3) {
    LoadCoreSourceLabelSet();
  }
  if (!m_targetSourceLHSJointCountFile.empty()) {
    LoadTargetSourceLeftHandSideJointCountFile();
  }
}

void SoftSourceSyntacticConstraintsFeature::LoadSourceLabelSet()
{
  VERBOSE(2, GetScoreProducerDescription() << ": Loading source label set from file " << m_sourceLabelSetFile << std::endl);
  InputFileStream inFile(m_sourceLabelSetFile);

  FactorCollection &factorCollection = FactorCollection::Instance();

  // read source label set
  std::string line;
  m_sourceLabels.clear();
  m_sourceLabelsByIndex.clear();
  m_sourceLabelsByIndex_RHS_1.clear();
  m_sourceLabelsByIndex_RHS_0.clear();
  m_sourceLabelsByIndex_LHS_1.clear();
  m_sourceLabelsByIndex_LHS_0.clear();
  m_sourceLabelIndexesByFactor.clear();
  while (getline(inFile, line)) {
    std::istringstream tokenizer(line);
    std::string label;
    size_t index;
    try {
      tokenizer >> label >> index;
    } catch (const std::exception &e) {
      UTIL_THROW2(GetScoreProducerDescription()
                  << ": Error reading source label set file " << m_sourceLabelSetFile << " .");
    }
    std::pair< boost::unordered_map<std::string,size_t>::iterator, bool > inserted = m_sourceLabels.insert( std::pair<std::string,size_t>(label,index) );
    UTIL_THROW_IF2(!inserted.second, GetScoreProducerDescription()
                   << ": Source label set file " << m_sourceLabelSetFile << " should contain each syntactic label only once.");

    if (index >= m_sourceLabelsByIndex.size()) {
      m_sourceLabelsByIndex.resize(index+1);
      m_sourceLabelsByIndex_RHS_1.resize(index+1);
      m_sourceLabelsByIndex_RHS_0.resize(index+1);
      m_sourceLabelsByIndex_LHS_1.resize(index+1);
      m_sourceLabelsByIndex_LHS_0.resize(index+1);
    }
    m_sourceLabelsByIndex[index] = label;
    m_sourceLabelsByIndex_RHS_1[index] = "RHS_1_" + label;
    m_sourceLabelsByIndex_RHS_0[index] = "RHS_0_" + label;
    m_sourceLabelsByIndex_LHS_1[index] = "LHS_1_" + label;
    m_sourceLabelsByIndex_LHS_0[index] = "LHS_0_" + label;
    const Factor* sourceLabelFactor = factorCollection.AddFactor(label,true);
    m_sourceLabelIndexesByFactor[sourceLabelFactor] = index;
  }

  inFile.Close();

  std::list<std::string> specialLabels;
  specialLabels.push_back("GlueTop");
  specialLabels.push_back("GlueX");
//  specialLabels.push_back("XRHS");
//  specialLabels.push_back("XLHS");
  for (std::list<std::string>::const_iterator iter=specialLabels.begin();
       iter!=specialLabels.end(); ++iter) {
    boost::unordered_map<std::string,size_t>::iterator found = m_sourceLabels.find(*iter);
    UTIL_THROW_IF2(found == m_sourceLabels.end(), GetScoreProducerDescription()
                   << ": Source label set file " << m_sourceLabelSetFile << " should contain an entry for the special label \"" << *iter << "\".");
    if (!(found->first).compare("GlueTop")) {
      m_GlueTopLabel = found->second;
//    } else if (!(found->first).compare("XRHS")) {
//      m_XRHSLabel = found->second;
//    } else if (!(found->first).compare("XLHS")) {
//      m_XLHSLabel = found->second;
    }
  }
}

void SoftSourceSyntacticConstraintsFeature::LoadCoreSourceLabelSet()
{
  VERBOSE(2, GetScoreProducerDescription() << ": Loading core source label set from file " << m_coreSourceLabelSetFile << std::endl);
  InputFileStream inFile(m_coreSourceLabelSetFile);

  // read core source label set
  std::string line;
  m_coreSourceLabels.clear();
  while (getline(inFile, line)) {
    istringstream tokenizer(line);
    std::string label;
    tokenizer >> label;
    boost::unordered_map<std::string,size_t>::iterator foundSourceLabelIndex = m_sourceLabels.find( label );
    if ( foundSourceLabelIndex != m_sourceLabels.end() ) {
      m_coreSourceLabels.insert(foundSourceLabelIndex->second);
    } else {
      VERBOSE(2, GetScoreProducerDescription()
              << ": Ignoring unknown source label \"" << label << "\" "
              << "from core source label set file " << m_coreSourceLabelSetFile << "."
              << std::endl);
    }
  }

  inFile.Close();
}

void SoftSourceSyntacticConstraintsFeature::LoadTargetSourceLeftHandSideJointCountFile()
{

  VERBOSE(2, GetScoreProducerDescription() << ": Loading target/source label joint counts from file " << m_targetSourceLHSJointCountFile << std::endl);
  InputFileStream inFile(m_targetSourceLHSJointCountFile);

  for (boost::unordered_map<const Factor*, std::vector< std::pair<float,float> >* >::iterator iter=m_labelPairProbabilities.begin();
       iter!=m_labelPairProbabilities.end(); ++iter) {
    delete iter->second;
  }
  m_labelPairProbabilities.clear();

  // read joint counts
  std::string line;
  FactorCollection &factorCollection = FactorCollection::Instance();
  boost::unordered_map<const Factor*,float> targetLHSCounts;
  std::vector<float> sourceLHSCounts(m_sourceLabels.size(),0.0);

  while (getline(inFile, line)) {
    istringstream tokenizer(line);
    std::string targetLabel;
    std::string sourceLabel;
    float count;
    tokenizer >> targetLabel;
    tokenizer >> sourceLabel;
    tokenizer >> count;

    boost::unordered_map<std::string,size_t>::iterator foundSourceLabelIndex = m_sourceLabels.find( sourceLabel );
    UTIL_THROW_IF2(foundSourceLabelIndex == m_sourceLabels.end(), GetScoreProducerDescription()
                   << ": Target/source label joint count file " << m_targetSourceLHSJointCountFile
                   << " contains unknown source label \"" << sourceLabel << "\".");

    const Factor* targetLabelFactor = factorCollection.AddFactor(targetLabel,true);

    sourceLHSCounts[foundSourceLabelIndex->second] += count;
    std::pair< boost::unordered_map<const Factor*,float >::iterator, bool > insertedTargetLHSCount =
      targetLHSCounts.insert( std::pair<const Factor*,float>(targetLabelFactor,count) );
    if (!insertedTargetLHSCount.second) {
      (insertedTargetLHSCount.first)->second += count;
      boost::unordered_map<const Factor*, std::vector< std::pair<float,float> >* >::iterator jointCountIt =
        m_labelPairProbabilities.find( targetLabelFactor );
      assert(jointCountIt != m_labelPairProbabilities.end());
      (jointCountIt->second)->at(foundSourceLabelIndex->second).first += count;
      (jointCountIt->second)->at(foundSourceLabelIndex->second).second += count;
    } else {
      std::pair<float,float> init(0.0,0.0);
      std::vector< std::pair<float,float> >* sourceVector = new std::vector< std::pair<float,float> >(m_sourceLabels.size(),init);
      sourceVector->at(foundSourceLabelIndex->second) = std::pair<float,float>(count,count);
      std::pair< boost::unordered_map<const Factor*, std::vector< std::pair<float,float> >* >::iterator, bool > insertedJointCount =
        m_labelPairProbabilities.insert( std::pair<const Factor*, std::vector< std::pair<float,float> >* >(targetLabelFactor,sourceVector) );
      assert(insertedJointCount.second);
    }
  }

  // normalization
  for (boost::unordered_map<const Factor*, std::vector< std::pair<float,float> >* >::iterator iter=m_labelPairProbabilities.begin();
       iter!=m_labelPairProbabilities.end(); ++iter) {
    float targetLHSCount = 0;
    boost::unordered_map<const Factor*,float >::const_iterator targetLHSCountIt = targetLHSCounts.find( iter->first );
    if ( targetLHSCountIt != targetLHSCounts.end() ) {
      targetLHSCount = targetLHSCountIt->second;
    }
    std::vector< std::pair<float,float> > &probabilities = *(iter->second);
    for (size_t index=0; index<probabilities.size(); ++index) {

      if ( probabilities[index].first != 0 ) {
        assert(targetLHSCount != 0);
        probabilities[index].first  /= targetLHSCount;
      }
      if ( probabilities[index].second != 0 ) {
        assert(sourceLHSCounts[index] != 0);
        probabilities[index].second /= sourceLHSCounts[index];
      }
    }
  }

  inFile.Close();
}


void SoftSourceSyntacticConstraintsFeature::EvaluateWithSourceContext(const InputType &input
    , const InputPath &inputPath
    , const TargetPhrase &targetPhrase
    , const StackVec *stackVec
    , ScoreComponentCollection &scoreBreakdown
    , ScoreComponentCollection *estimatedFutureScore) const
{
  assert(stackVec);

  IFFEATUREVERBOSE(2) {
    FEATUREVERBOSE(2, targetPhrase << std::endl);
    FEATUREVERBOSE(2, inputPath << std::endl);
    for (size_t i = 0; i < stackVec->size(); ++i) {
      const ChartCellLabel &cell = *stackVec->at(i);
      const WordsRange &ntRange = cell.GetCoverage();
      FEATUREVERBOSE(2, "stackVec[ " << i << " ] : " << ntRange.GetStartPos() << " - " << ntRange.GetEndPos() << std::endl);
    }

    for (AlignmentInfo::const_iterator it=targetPhrase.GetAlignNonTerm().begin();
         it!=targetPhrase.GetAlignNonTerm().end(); ++it) {
      FEATUREVERBOSE(2, "alignNonTerm " << it->first << " " << it->second << std::endl);
    }
  }

  // dense scores
  std::vector<float> newScores(m_numScoreComponents,0); // m_numScoreComponents == 3

  const TreeInput& treeInput = static_cast<const TreeInput&>(input);
  const StaticData& staticData = StaticData::Instance();
  const Word& outputDefaultNonTerminal = staticData.GetOutputDefaultNonTerminal();

  size_t nNTs = 1;
  bool treeInputMismatchLHSBinary = true;
  size_t treeInputMismatchRHSCount = 0;
  bool hasCompleteTreeInputMatch = false;
  float t2sLabelsProb = 1;
  float s2tLabelsProb = 1;
  float ruleLabelledProbability = 1;

  // read SourceLabels property
  const Factor* targetLHS = targetPhrase.GetTargetLHS()[0];
  bool isGlueGrammarRule = false;
  bool isUnkRule = false;

  if (const PhraseProperty *property = targetPhrase.GetProperty("SourceLabels")) {

    const SourceLabelsPhraseProperty *sourceLabelsPhraseProperty = static_cast<const SourceLabelsPhraseProperty*>(property);

    nNTs = sourceLabelsPhraseProperty->GetNumberOfNonTerminals();
    float totalCount = sourceLabelsPhraseProperty->GetTotalCount();

    // prepare for input tree label matching
    std::vector< boost::unordered_set<size_t> > treeInputLabelsRHS(nNTs-1);
    boost::unordered_set<size_t> treeInputLabelsLHS;

    // get index map for underlying hypotheses
    const AlignmentInfo::NonTermIndexMap &nonTermIndexMap =
      targetPhrase.GetAlignNonTerm().GetNonTermIndexMap();

    std::vector<const Factor*> targetLabelsRHS;
    if (nNTs > 1) { // rule has right-hand side non-terminals, i.e. it's a hierarchical rule
      size_t nonTerminalNumber = 0;

      for (size_t phrasePos=0; phrasePos<targetPhrase.GetSize(); ++phrasePos) {
        // consult rule for either word or non-terminal
        const Word &word = targetPhrase.GetWord(phrasePos);
        if ( word.IsNonTerminal() ) {
          // non-terminal: consult subderivation
          size_t nonTermIndex = nonTermIndexMap[phrasePos];
          targetLabelsRHS.push_back( word[0] );

          // retrieve information that is required for input tree label matching (RHS)
          const ChartCellLabel &cell = *stackVec->at(nonTermIndex);
          const WordsRange& prevWordsRange = cell.GetCoverage();
          size_t prevStartPos = prevWordsRange.GetStartPos();
          size_t prevEndPos = prevWordsRange.GetEndPos();
          const NonTerminalSet& prevTreeInputLabels = treeInput.GetLabelSet(prevStartPos,prevEndPos);

          for (NonTerminalSet::const_iterator prevTreeInputLabelsIt = prevTreeInputLabels.begin();
               prevTreeInputLabelsIt != prevTreeInputLabels.end(); ++prevTreeInputLabelsIt) {
            if (*prevTreeInputLabelsIt != outputDefaultNonTerminal) {
              boost::unordered_map<const Factor*,size_t>::const_iterator foundPrevTreeInputLabel
              = m_sourceLabelIndexesByFactor.find((*prevTreeInputLabelsIt)[0]);
              if (foundPrevTreeInputLabel != m_sourceLabelIndexesByFactor.end()) {
                size_t prevTreeInputLabelIndex = foundPrevTreeInputLabel->second;
                treeInputLabelsRHS[nonTerminalNumber].insert(prevTreeInputLabelIndex);
              }
            }
          }

          ++nonTerminalNumber;
        }
      }
    }

    // retrieve information that is required for input tree label matching (LHS)
    const WordsRange& wordsRange = inputPath.GetWordsRange();
    size_t startPos = wordsRange.GetStartPos();
    size_t endPos = wordsRange.GetEndPos();
    const NonTerminalSet& treeInputLabels = treeInput.GetLabelSet(startPos,endPos);

    for (NonTerminalSet::const_iterator treeInputLabelsIt = treeInputLabels.begin();
         treeInputLabelsIt != treeInputLabels.end(); ++treeInputLabelsIt) {
      if (*treeInputLabelsIt != outputDefaultNonTerminal) {
        boost::unordered_map<const Factor*,size_t>::const_iterator foundTreeInputLabel
        = m_sourceLabelIndexesByFactor.find((*treeInputLabelsIt)[0]);
        if (foundTreeInputLabel != m_sourceLabelIndexesByFactor.end()) {
          size_t treeInputLabelIndex = foundTreeInputLabel->second;
          treeInputLabelsLHS.insert(treeInputLabelIndex);
        }
      }
    }


    // inspect source-labelled rule items

    std::vector< boost::unordered_set<size_t> > sparseScoredTreeInputLabelsRHS(nNTs-1);
    boost::unordered_set<size_t> sparseScoredTreeInputLabelsLHS;

    std::vector<bool> sourceLabelSeenAsLHS(m_sourceLabels.size(),false);
    std::vector<bool> treeInputMatchRHSCountByNonTerminal(nNTs-1,false);

    const std::list<SourceLabelsPhrasePropertyItem> &sourceLabelItems = sourceLabelsPhraseProperty->GetSourceLabelItems();

    for (std::list<SourceLabelsPhrasePropertyItem>::const_iterator sourceLabelItem = sourceLabelItems.begin();
         sourceLabelItem != sourceLabelItems.end() && !hasCompleteTreeInputMatch; ++sourceLabelItem) {

      const std::list<size_t> &sourceLabelsRHS = sourceLabelItem->GetSourceLabelsRHS();
      // float sourceLabelsRHSCount = sourceLabelItem->GetSourceLabelsRHSCount();
      const std::list< std::pair<size_t,float> > &sourceLabelsLHSList = sourceLabelItem->GetSourceLabelsLHSList();

      assert(sourceLabelsRHS.size() == nNTs-1);

      bool currentSourceLabelItemIsCompleteTreeInputMatch = true;

      size_t nonTerminalNumber=0;
      for (std::list<size_t>::const_iterator sourceLabelsRHSIt = sourceLabelsRHS.begin();
           sourceLabelsRHSIt != sourceLabelsRHS.end(); ++sourceLabelsRHSIt, ++nonTerminalNumber) {

        if (treeInputLabelsRHS[nonTerminalNumber].find(*sourceLabelsRHSIt) != treeInputLabelsRHS[nonTerminalNumber].end()) {

          treeInputMatchRHSCountByNonTerminal[nonTerminalNumber] = true;

          if ( m_featureVariant == 2 ||
               (m_featureVariant == 3 && m_coreSourceLabels.find(*sourceLabelsRHSIt) != m_coreSourceLabels.end()) ) {
            // score sparse features: RHS match
            if (sparseScoredTreeInputLabelsRHS[nonTerminalNumber].find(*sourceLabelsRHSIt) == sparseScoredTreeInputLabelsRHS[nonTerminalNumber].end()) {
              // (only if no match has been scored for this tree input label and rule non-terminal with a previous sourceLabelItem)
              float score_RHS_1 = (float)1/treeInputLabelsRHS[nonTerminalNumber].size();
              scoreBreakdown.PlusEquals(this,
                                        m_sourceLabelsByIndex_RHS_1[*sourceLabelsRHSIt],
                                        score_RHS_1);
              sparseScoredTreeInputLabelsRHS[nonTerminalNumber].insert(*sourceLabelsRHSIt);
            }
          }

        } else {

          currentSourceLabelItemIsCompleteTreeInputMatch = false;

        }
      }

      // LHS source non-terminal labels seen with this RHS
      bool currentSourceLabelItemHasLHSTreeInputMatch = false;
      //float ruleLabelledCount = 0;
      std::list< std::pair<size_t,float> >::const_iterator sourceLabelsLHSIt;

      for (sourceLabelsLHSIt = sourceLabelsLHSList.begin(); sourceLabelsLHSIt != sourceLabelsLHSList.end(); ++sourceLabelsLHSIt) {

        if ( sourceLabelsLHSIt->first == m_GlueTopLabel ) {
          isGlueGrammarRule = true;
        }

        if (treeInputLabelsLHS.find(sourceLabelsLHSIt->first) != treeInputLabelsLHS.end()) {

          currentSourceLabelItemHasLHSTreeInputMatch = true;

          if ( m_featureVariant == 2 ||
               (m_featureVariant == 3 && m_coreSourceLabels.find(sourceLabelsLHSIt->first) != m_coreSourceLabels.end()) ) {
            // score sparse features: LHS match
            if (sparseScoredTreeInputLabelsLHS.find(sourceLabelsLHSIt->first) == sparseScoredTreeInputLabelsLHS.end()) {
              // (only if no match has been scored for this tree input label and rule non-terminal with a previous sourceLabelItem)
              float score_LHS_1 = (float)1/treeInputLabelsLHS.size();
              scoreBreakdown.PlusEquals(this,
                                        m_sourceLabelsByIndex_LHS_1[sourceLabelsLHSIt->first],
                                        score_LHS_1);
              sparseScoredTreeInputLabelsLHS.insert(sourceLabelsLHSIt->first);
            }
          }
          break;

        }
      }

      if (currentSourceLabelItemHasLHSTreeInputMatch) {
        // input tree matching (LHS)
        treeInputMismatchLHSBinary = false;
      } else {
        currentSourceLabelItemIsCompleteTreeInputMatch = false;
      }

      if (currentSourceLabelItemIsCompleteTreeInputMatch) {
        hasCompleteTreeInputMatch = true;

        ruleLabelledProbability = sourceLabelsLHSIt->second / totalCount;
        std::pair<float,float> probPair = GetLabelPairProbabilities( targetLHS, sourceLabelsLHSIt->first);
        t2sLabelsProb = probPair.first;
        s2tLabelsProb = probPair.second;
        nonTerminalNumber=0;
        for (std::list<size_t>::const_iterator sourceLabelsRHSIt = sourceLabelsRHS.begin();
             sourceLabelsRHSIt != sourceLabelsRHS.end(); ++sourceLabelsRHSIt, ++nonTerminalNumber) {
          probPair = GetLabelPairProbabilities( targetLabelsRHS[nonTerminalNumber], *sourceLabelsRHSIt );
          t2sLabelsProb += probPair.first;
          s2tLabelsProb += probPair.second;
        }
        t2sLabelsProb /= nNTs;
        s2tLabelsProb /= nNTs;
        assert(t2sLabelsProb != 0);
        assert(s2tLabelsProb != 0);
      }

    }

    // input tree matching (RHS)
    if ( !hasCompleteTreeInputMatch ) {
      treeInputMismatchRHSCount = nNTs-1;
      for (std::vector<bool>::const_iterator treeInputMatchRHSCountByNonTerminalIt = treeInputMatchRHSCountByNonTerminal.begin();
           treeInputMatchRHSCountByNonTerminalIt != treeInputMatchRHSCountByNonTerminal.end(); ++treeInputMatchRHSCountByNonTerminalIt) {
        if (*treeInputMatchRHSCountByNonTerminalIt) {
          --treeInputMismatchRHSCount;
        }
      }
    }

    // score sparse features: mismatches
    if ( m_featureVariant == 2 || m_featureVariant == 3 ) {

      // RHS

      for (size_t nonTerminalNumber = 0; nonTerminalNumber < nNTs-1; ++nonTerminalNumber) {
        // nNTs-1 because nNTs also counts the left-hand side non-terminal

        float score_RHS_0 = (float)1/treeInputLabelsRHS[nonTerminalNumber].size();
        for (boost::unordered_set<size_t>::const_iterator treeInputLabelsRHSIt = treeInputLabelsRHS[nonTerminalNumber].begin();
             treeInputLabelsRHSIt != treeInputLabelsRHS[nonTerminalNumber].end(); ++treeInputLabelsRHSIt) {

          if ( m_featureVariant == 2 ||
               (m_featureVariant == 3 && m_coreSourceLabels.find(*treeInputLabelsRHSIt) != m_coreSourceLabels.end()) ) {

            if (sparseScoredTreeInputLabelsRHS[nonTerminalNumber].find(*treeInputLabelsRHSIt) == sparseScoredTreeInputLabelsRHS[nonTerminalNumber].end()) {
              // score sparse features: RHS mismatch
              scoreBreakdown.PlusEquals(this,
                                        m_sourceLabelsByIndex_RHS_0[*treeInputLabelsRHSIt],
                                        score_RHS_0);
            }
          }
        }
      }

      // LHS

      float score_LHS_0 = (float)1/treeInputLabelsLHS.size();
      for (boost::unordered_set<size_t>::const_iterator treeInputLabelsLHSIt = treeInputLabelsLHS.begin();
           treeInputLabelsLHSIt != treeInputLabelsLHS.end(); ++treeInputLabelsLHSIt) {

        if ( m_featureVariant == 2 ||
             (m_featureVariant == 3 && m_coreSourceLabels.find(*treeInputLabelsLHSIt) != m_coreSourceLabels.end()) ) {

          if (sparseScoredTreeInputLabelsLHS.find(*treeInputLabelsLHSIt) == sparseScoredTreeInputLabelsLHS.end()) {
            // score sparse features: RHS mismatch
            scoreBreakdown.PlusEquals(this,
                                      m_sourceLabelsByIndex_LHS_0[*treeInputLabelsLHSIt],
                                      score_LHS_0);
          }
        }
      }

    }

  } else {

    // abort with error message if the phrase does not translate an unknown word
    UTIL_THROW_IF2(!targetPhrase.GetWord(0).IsOOV(), GetScoreProducerDescription()
                   << ": Missing SourceLabels property. "
                   << "Please check phrase table and glue rules.");

    // unknown word
    isUnkRule = true;

  }

  // add scores

  // input tree matching
  switch (m_featureVariant) {

  case 0:
    newScores[0] = hasCompleteTreeInputMatch;
    break;

  case 1:
    newScores[0] = ( (hasCompleteTreeInputMatch || isGlueGrammarRule || isUnkRule) ? 0 : std::numeric_limits<float>::min() );
    break;

  default:
    newScores[0] = hasCompleteTreeInputMatch;
  }
  newScores[1] = treeInputMismatchLHSBinary;
  newScores[2] = treeInputMismatchRHSCount;
//  newScores[3] = hasCompleteTreeInputMatch ? std::log(t2sLabelsProb) : 0;
//  newScores[4] = hasCompleteTreeInputMatch ? std::log(s2tLabelsProb) : 0;
//  newScores[3] = hasCompleteTreeInputMatch ? std::log(ruleLabelledProbability) : 0;

  scoreBreakdown.PlusEquals(this, newScores);
}


std::pair<float,float> SoftSourceSyntacticConstraintsFeature::GetLabelPairProbabilities(
  const Factor* target,
  const size_t source) const
{
  boost::unordered_map<const Factor*, std::vector< std::pair<float,float> >* >::const_iterator found =
    m_labelPairProbabilities.find(target);
  if ( found == m_labelPairProbabilities.end() ) {
    return std::pair<float,float>(0,0);
  }
  return found->second->at(source);
}


}

