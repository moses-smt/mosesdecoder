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

using namespace std;

namespace Moses
{

VWInstance vwInstance;

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
  std::cout << "Initializing vw..." << std::endl;
  vwInstance.m_vw = VW::initialize("--hash all -q st --noconstant -i " + modelFile);
  return LoadRuleIndex(indexFile);
}

vector<string> CellContextScoreProducer::GetSourceFeatures(
  const InputType &srcSent,
  const std::string &sourceSide)
{

  std::cout << "Getting source features : " << srcSent.GetSize() << std::endl;
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
                                                                      const InputType &source
                                                                      )
{

  std::cout << "Scoring Rules... " << sourceSide << " : " << targetRepresentations->front() << " : "<< source << std::endl;

  //FB : debugging : check what in index map :
  RuleIndexType :: iterator itr_rule_index;
  for(itr_rule_index = m_ruleIndex.begin(); itr_rule_index != m_ruleIndex.end(); itr_rule_index++)
  {
      std::cout << "Index : " << itr_rule_index->first << " : " << itr_rule_index->second << std::endl;
  }
    vector<ScoreComponentCollection> scores;
  // iterator over target representations
  std::vector<std::string> :: iterator itr_targ_rep;
  float sum = 0;

    std::vector<string> sourceFeatures = GetSourceFeatures(source, sourceSide);

    //FB: for debugging
    std::vector<std::string> :: iterator itr_source_feat;
    for(itr_source_feat = sourceFeatures.begin(); itr_source_feat != sourceFeatures.end(); itr_source_feat++)
    {
        std::cout << *itr_source_feat << std::endl;
    }

    // create VW example, add source-side features
    ezexample ex(&vwInstance.m_vw, false);
    ex(vw_namespace('s'));
    std::vector<std::string>::const_iterator fIt;
    for (fIt = sourceFeatures.begin(); fIt != sourceFeatures.end(); fIt++) {
      std::cout << "Source Feature vw : " << *fIt << std::endl;
      ex.addf(*fIt);
    }

    float score = 0.0;
    for (itr_targ_rep = targetRepresentations->begin(); itr_targ_rep != targetRepresentations->end(); itr_targ_rep++) {

        std::cout << "Target representation : " << *itr_targ_rep << std::endl;
        std::cout << (m_ruleIndex.find(*itr_targ_rep) == m_ruleIndex.end()) <<  std::endl;

        if (! IsOOV(*itr_targ_rep) ) {

            std::cout << "Target Representation vw : " << *itr_targ_rep << std::endl;
            vector<string> targetFeatures = GetTargetFeatures(*itr_targ_rep);

            // set label to target phrase index
            ex.set_label(SPrint(m_ruleIndex[*itr_targ_rep]));

            // move to target namespace, add target phrase as a feature
            ex(vw_namespace('p'));
            for (fIt = targetFeatures.begin(); fIt != targetFeatures.end(); fIt++) {
            std::cout << "Target feature vw : " << *fIt << std::endl;
            ex.addf(*fIt);

            score = 1 / (1 + exp(-ex()));
            std::cout << "Score " << score << std::endl;

            }
        }
        else
        {
            score = 0;
            std::cout << *itr_targ_rep << "Is out of vocabulary : " << score << std::endl;
        }

        //    cerr << srcPhrase << " ||| " << tgtPhrase << " ||| " << score << endl;
        sum += score;

        // create score object
        ScoreComponentCollection scoreCol;
        scoreCol.Assign(this, score);
        scores.push_back(scoreCol);
        // move out of target namespace
        --ex;
    }
    // normalize
    if (sum != 0) {
        vector<ScoreComponentCollection>::iterator colIt;
        for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
        std::cout << "Added feature : " << colIt->GetScoreForProducer(this) << " : " << std::endl;
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
