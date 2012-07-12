// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "CellContextScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include "Util.h"
#include "vw.h"
#include "ezexample.h"
#include <vector>
#include <iostream>
#include <fstream>

#include "FeatureExtractor.h"
#include "FeatureConsumer.h"
using namespace std;

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

//note : initialized in static data ln.
bool CellContextScoreProducer::Initialize(const string &modelFile, const string &indexFile)
{
  bool isGood = LoadRuleIndex(indexFile);

  VERBOSE(4, "Initializing vw..." << endl);
  // configure features
  FeatureTypes ft;
  ft.m_sourceExternal = true;
  ft.m_sourceInternal = true;
  ft.m_targetInternal = true;
  ft.m_paired = false;
  ft.m_bagOfWords = false;
  ft.m_syntacticParent = true;
  ft.m_contextWindow = 2;
  ft.m_factors.push_back(0);
  ft.m_factors.push_back(1);
  ft.m_factors.push_back(2);

  FeatureExtractor m_extractor = new FeatureExtractor(ft,m_ruleIndex,0);
  FeatureConsumer m_consumer = new VWLibraryPredictConsumer(modelFile);
  //vwInstance.m_vw = VW::initialize("--hash all -q st --noconstant -i " + modelFile);
  return isGood;
}

vector<string> CellContextScoreProducer::GetSourceFeatures(
  const InputType &srcSent,
  const std::string &sourceSide)
{

  vector<string> out;

  // bag of words features
  for (size_t i = 0; i < srcSent.GetSize(); i++) {
    string word = srcSent.GetWord(i).GetString(m_srcFactors, false);
    out.push_back("w^" + word);
  }

  // phrase feature
  out.push_back("p^" + Replace(sourceSide, " ", "_"));

  return out;
}

vector<string> CellContextScoreProducer::GetTargetFeatures(const std::string &targetRep)
{
  vector<string> out;
  out.push_back("p^" + Replace(targetRep, " ", "_"));

  return out;
}

bool CellContextScoreProducer::IsOOV(const std::string &targetRep)
{
  return m_ruleIndex.find(targetRep) == m_ruleIndex.end();
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
    m_ruleIndex.insert(make_pair<string, size_t>(columns[0], idx));
  }
  in.close();
  return true;
}

vector<ScoreComponentCollection> CellContextScoreProducer::ScoreRules(
                                                                      const std::string &sourceSide,
                                                                      std::vector<std::string> * targetRepresentations,
                                                                      const InputType &source,
                                                                      size_t startSpan,
                                                                      size_t endSpan
                                                                      )
{
  //debugging : check that everything is fine in index map
  //RuleIndexType :: iterator itr_rule_index;
  //for(itr_rule_index = m_ruleIndex.begin(); itr_rule_index != m_ruleIndex.end(); itr_rule_index++)
  //{
  //    std::cout << "Index : " << itr_rule_index->first << " : " << itr_rule_index->second << std::endl;
  //}

    vector<ScoreComponentCollection> scores;
    // iterator over target representations
    std::vector<std::string> :: iterator itr_targ_rep;
    float sum = 0;

    //FB: for debugging
    //std::vector<std::string> :: iterator itr_source_feat;
    //for(itr_source_feat = sourceFeatures.begin(); itr_source_feat != sourceFeatures.end(); itr_source_feat++)
    //{
    //    std::cout << *itr_source_feat << std::endl;
    //}

    float score = 0.0;
    ContextType contextType;
    vector<std::string> tokSourceSide = Tokenize(" ",sourceSide);
    m_extractor.GenerateFeatures(m_fc,contextType,tokSourceSide,spanStart,spanEnd,targetRepresentations,losses);
    vector<std::string> * :: iterator itr_target_rep;
    for(itr_target_rep = targetRepresentations.begin(); itr_target_rep != targetRepresentations,end(); itr_target_rep++)
    {
        score = m_consumer.predict(**itr_target_rep);
        //    cerr << srcPhrase << " ||| " << tgtPhrase << " ||| " << score << endl;
        sum += score;

        // create score object
        ScoreComponentCollection scoreCol;
        scoreCol.Assign(this, score);
        scores.push_back(scoreCol);
        //std::cout << "Collection before normalization : " << scoreCol << std::endl;
    }
    // normalize
    if (sum != 0) {
        vector<ScoreComponentCollection>::iterator colIt;
        for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
        //std::cout << "Normalizing : " << colIt->GetScoreForProducer(this) << " : " << std::endl;
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
