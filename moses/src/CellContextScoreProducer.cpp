// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "CellContextScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include "Util.h"
#include <vector>
#include <iostream>
#include <fstream>

#include "FeatureExtractor.h"
#include "FeatureConsumer.h"

using namespace std;
using namespace PSD;

namespace Moses
{

CellContextScoreProducer::CellContextScoreProducer(ScoreIndexManager &scoreIndexManager, float weight)
{
  scoreIndexManager.AddScoreProducer(this);
  vector<float> weights;
  weights.push_back(weight);
  m_srcFactors.push_back(0);
  m_tgtFactors.push_back(0);
  const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
}

CellContextScoreProducer::~CellContextScoreProducer()
{
    delete m_extractor;
    delete m_consumerFactory;
}

bool CellContextScoreProducer::Initialize(const string &modelFile, const string &indexFile, const string &configFile)
{
  bool isGood = LoadRuleIndex(indexFile);

  m_consumerFactory = new VWLibraryPredictConsumerFactory(modelFile, 255);
  if (! LoadRuleIndex(indexFile))
  isGood = false;

  m_extractorConfig.Load(configFile);

  m_extractor = new FeatureExtractor(m_ruleIndex, m_extractorConfig, false);
  isGood = true;
  VERBOSE(4, "Constructing score producers : " << isGood << endl);
  return isGood;
}

ScoreComponentCollection CellContextScoreProducer::ScoreFactory(float score)
{
  ScoreComponentCollection out;
  out.Assign(this, score);
  return out;
}

void CellContextScoreProducer::CheckIndex(const std::string &targetRep)
{
  if (m_ruleIndex.left.find(targetRep) == m_ruleIndex.left.end())
    throw runtime_error("Phrase not in index: " + targetRep);
}

bool CellContextScoreProducer::IsOOV(const std::string &targetRep)
{
  return m_ruleIndex.left.find(targetRep) == m_ruleIndex.left.end();
}

Translation CellContextScoreProducer::GetPSDTranslation(const TargetPhrase *tp)
{

  VERBOSE(5, "Target Phrase score before adding stateless : " << (*tp) << " : " << tp->GetFutureScore() << std::endl);
  Translation psdOpt;

  // phrase ID
  string tgtPhrase = tp->GetStringRep(m_tgtFactors);
  psdOpt.m_index = m_ruleIndex.left.find(tgtPhrase)->second;

  // alignment
  const AlignmentInfo &alignInfo = tp->GetWordAlignmentInfo();
  AlignmentInfo::const_iterator it;
  for (it = alignInfo.begin(); it != alignInfo.end(); it++)
    psdOpt.m_alignment.insert(*it);

  // scores
  const TranslationSystem& system = StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);
  const vector<PhraseDictionaryFeature*>& ttables = system.GetPhraseDictionaries();
  const ScoreComponentCollection &scoreCollection = tp->GetScoreBreakdown();
  psdOpt.m_scores = scoreCollection.GetScoresForProducer(ttables[0]); // assuming one translation step!

  return psdOpt;
}


bool CellContextScoreProducer::LoadRuleIndex(const string &indexFile)
{
  ifstream in(indexFile.c_str());
  if (!in.good())
    return false;
  string line;
  while (getline(in, line)) {
    vector<string> columns = Tokenize(line, "\t");
    size_t idx = Scan<size_t>(columns[1]);
    m_ruleIndex.insert(TargetIndexType::value_type(columns[0], idx));
  }
  in.close();
  return true;
}

vector<ScoreComponentCollection> CellContextScoreProducer::ScoreRules(
                                                                        size_t startSpan,
                                                                        size_t endSpan,
                                                                        const std::string &sourceSide,
                                                                        std::vector<std::string> * targetRepresentations,
                                                                        const InputType &source,
                                                                        map<string,TargetPhrase*> * targetMap
                                                                      )
{
  //debugging : check that everything is fine in index map
  //RuleIndexType :: iterator itr_rule_index;
  //for(itr_rule_index = m_ruleIndex.begin(); itr_rule_index != m_ruleIndex.end(); itr_rule_index++)
  //{
  //    std::cout << "Index : " << itr_rule_index->first << " : " << itr_rule_index->second << std::endl;
  //}

    vector<ScoreComponentCollection> scores;
    float sum = 0.0;
    float score = 0.0;

    //If the source is OOV, it will be copied into target
    if (! IsOOV(targetRepresentations->front()))
    {

        vector<float> losses(targetRepresentations->size());
        vector<Translation> psdOptions;

        map<string,TargetPhrase*> :: iterator itr_rep;
        vector<std::string>::const_iterator tgtRepIt;
        for (tgtRepIt = targetRepresentations->begin(); tgtRepIt != targetRepresentations->end(); tgtRepIt++) {
          CHECK(targetMap->find(*tgtRepIt) != targetMap->end());
          itr_rep = targetMap->find(*tgtRepIt);
          CheckIndex(*tgtRepIt);
          psdOptions.push_back(GetPSDTranslation(itr_rep->second));
        }


        VERBOSE(5, "Extracting features features for source : " << sourceSide << endl);
        //damt hiero : extract syntax features
        vector<std::string> syntaxFeats;
        NonTerminalSet labelSet = source.GetLabelSet(startSpan,endSpan);
        //check if there is a label for this span
        std::string noTag = "X";
        std::string syntFeat;
        if(labelSet.size() == 0)
        {
            syntaxFeats.push_back(noTag);
            VERBOSE(6, "Added syntax label : " << noTag << endl);
        }
        else
        {
            NonTerminalSet::const_iterator itr_label;
            for(itr_label = labelSet.begin(); itr_label != labelSet.end(); itr_label++)
            {
                Word label = *itr_label;
                CHECK(label.IsNonTerminal() == 1);
                syntFeat = label.GetString(m_srcFactors,0);
                syntaxFeats.push_back(syntFeat);
                VERBOSE(6, "Added syntax label : " << syntFeat << endl);

            }

        }
        VWLibraryPredictConsumer * p_consumer = m_consumerFactory->Acquire();
        m_extractor->GenerateFeaturesChart(p_consumer,source.m_PSDContext,sourceSide,syntaxFeats,startSpan,endSpan,psdOptions,losses);
        m_consumerFactory->Release(p_consumer);

        vector<float>::iterator lossIt;
        for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
        VERBOSE(5, "Obtained prediction : " << sourceSide << endl);
        *lossIt = exp(-*lossIt);
        //put the score into scores
        scores.push_back(ScoreFactory(*lossIt));
        sum += *lossIt;
        }
    }
    else {
    for (size_t i = 0; i < targetRepresentations->size(); i++) {
      scores.push_back(ScoreFactory(0));}
    }
    // normalize
    if (sum != 0) {
        vector<ScoreComponentCollection>::iterator colIt;
        for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
        colIt->Assign(this, log(colIt->GetScoreForProducer(this) / sum));
        }
    }
    //normalize
    return scores;


}

size_t CellContextScoreProducer::GetNumScoreComponents() const
{
  return 1; // let's return just P(e|f) for now
}

std::string CellContextScoreProducer::GetScoreProducerDescription(unsigned) const
{
  return "CellContext";
}

std::string CellContextScoreProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "cc";
}

size_t CellContextScoreProducer::GetNumInputScores() const
{
  return 0;
}

} // namespace Moses
