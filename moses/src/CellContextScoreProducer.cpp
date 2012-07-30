// $Id$

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "CellContextScoreProducer.h"
#include "WordsRange.h"
#include "TranslationOption.h"
#include "Util.h"
#include "../../phrase-extract/extract-syntax-features/InputTreeRep.h"
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

ChartTranslation CellContextScoreProducer::GetPSDTranslation(const string targetRep, const TargetPhrase *tp)
{
  VERBOSE(5, "Target Phrase put into translation vector : " << (*tp) << " : " << tp->GetFutureScore() << std::endl);
  ChartTranslation psdOpt;

  // phrase ID
  VERBOSE(6, "LOOKED UP TARGET REP : " << targetRep << endl);
  CHECK(m_ruleIndex.left.find(targetRep) != m_ruleIndex.left.end());
  psdOpt.m_index = m_ruleIndex.left.find(targetRep)->second;
  VERBOSE(6, "FOUND INDEX : " << m_ruleIndex.left.find(targetRep)->second << endl);

  //alignment between terminals and non-terminals
  // alignment between terminals
  const AlignmentInfo &alignInfoTerm = tp->GetWordAlignmentInfo();
  VERBOSE(5, "Added alignment Info : " << alignInfoTerm << std::endl);
  AlignmentInfo::const_iterator it;
  for (it = alignInfoTerm.begin(); it != alignInfoTerm.end(); it++)
    //cerr << "Added Alignment : " << (*it) << endl;
    psdOpt.m_termAlignment.insert(*it);

  //alignment between non-terminals
  const AlignmentInfo &alignInfoNonTerm = tp->GetAlignmentInfo();
  VERBOSE(5, "Added alignment Info between non terms : " << alignInfoNonTerm << std::endl);
  VERBOSE(5, "Added alignment Info between terms : " << alignInfoTerm << std::endl);
  for (it = alignInfoNonTerm.begin(); it != alignInfoNonTerm.end(); it++)
    //cerr << "Added Alignment : " << (*it) << endl;
    psdOpt.m_nonTermAlignment.insert(*it);

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
  size_t index = 0;
  while (getline(in, line)) {
    m_ruleIndex.insert(TargetIndexType::value_type(line, ++index));
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
    string span;

    //get span
    int spanSize = (endSpan-startSpan) + 1;
    stringstream s;
    s << spanSize;
    span = s.str();

    if(targetRepresentations->size() > 1)
    {
        vector<float> losses(targetRepresentations->size());
        vector<ChartTranslation> psdOptions;

        map<string,TargetPhrase*> :: iterator itr_rep;
        vector<std::string>::const_iterator tgtRepIt;
        for (tgtRepIt = targetRepresentations->begin(); tgtRepIt != targetRepresentations->end(); tgtRepIt++) {
          CHECK(targetMap->find(*tgtRepIt) != targetMap->end());
          itr_rep = targetMap->find(*tgtRepIt);
          VERBOSE(6, "CHECKING INDEX FOR : " << *tgtRepIt << endl);
          CheckIndex(*tgtRepIt);

          psdOptions.push_back(GetPSDTranslation(*tgtRepIt,itr_rep->second));
        }

        VERBOSE(5, "Extracting features for source : " << sourceSide << endl);
        VERBOSE(5, "Extracting features for spans : " << startSpan << " : " << endSpan << endl);
        //damt hiero : extract syntax features
        //print chart
        size_t sizeOfSource = source.GetSize();
        //std::cerr << "SIZE OF SOURCE : " << sizeOfSource << std::endl;
        //source.GetInputTreeRep()->Print(sizeOfSource);

        //std::cerr << "GETTING SYNTAX LABELS" << std::endl;
        bool IsBegin = false;
        vector<SyntaxLabel> syntaxLabels = source.GetInputTreeRep()->GetLabels(startSpan,endSpan);
        //std::cerr << "GETTING PARENT" << std::endl;
        SyntaxLabel parentLabel = source.GetInputTreeRep()->GetParent(startSpan,endSpan,IsBegin);
        vector<string> syntFeats;


        //std::cerr << "LOOPING THROUGH PARENT : " << startSpan << " : " << endSpan << std::endl;

        //damt hiero : TODO : use GetNoTag : also in extract-syntax features
        string noTag = "NOTAG";
        IsBegin = false;
        while(!parentLabel.GetString().compare("NOTAG"))
        {
            //cerr << "LOOKING FOR PARENT OF : " << parentLabel.GetString() << endl;
            parentLabel = source.GetInputTreeRep()->GetParent(startSpan,endSpan,IsBegin);
            //cerr << "FOUND PARENT : " << parentLabel.GetString() << endl;
            //cerr << "BEGIN OF CHART : " << IsBegin << endl;
            if( !(IsBegin ) )
            {startSpan--;}
            else
            {endSpan++;}
        }

        //iterate over labels and get strings
        //MAYBE INEFFICIENT
        vector<SyntaxLabel>::iterator itr_syn_lab;
        for(itr_syn_lab = syntaxLabels.begin(); itr_syn_lab != syntaxLabels.end(); itr_syn_lab++)
        {
            SyntaxLabel syntaxLabel = *itr_syn_lab;
            CHECK(syntaxLabel.IsNonTerm() == 1);
            string syntFeat = syntaxLabel.GetString();

            bool toRemove = false;
            if( (syntaxLabels.size() > 1 ) && !(syntFeat.compare( source.GetInputTreeRep()->GetNoTag() )) )
            {toRemove = true;}

            if(toRemove == false)
            {
                syntFeats.push_back(syntFeat);
            }
        }

        VWLibraryPredictConsumer * p_consumer = m_consumerFactory->Acquire();
        m_extractor->GenerateFeaturesChart(p_consumer,source.m_PSDContext,sourceSide,syntFeats,parentLabel.GetString(),span,startSpan,endSpan,psdOptions,losses);
        m_consumerFactory->Release(p_consumer);

        vector<float>::iterator lossIt;
        for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
        VERBOSE(5, "Obtained prediction : " << sourceSide << endl);
        *lossIt = exp(-*lossIt);
        VERBOSE(4, "Obtained score : " <<  *lossIt  << endl);
        //put the score into scores
        scores.push_back(ScoreFactory(*lossIt));
        sum += *lossIt;
        VERBOSE(4, "Sum to normalize" << sum << std::endl);
        }

        // normalize
        if (sum != 0) {
            vector<ScoreComponentCollection>::iterator colIt;
            for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
            VERBOSE(5, "Score before normalizing : " << *colIt << std::endl);
            colIt->Assign(this, log(colIt->GetScoreForProducer(this) / sum));
            VERBOSE(5, "Score after normalizing : " << *colIt << std::endl);
            }
        }
        else
        {
            VERBOSE(5, "SUM IS ZERO : SCORES PUT TO 0 " << std::endl);
            vector<ScoreComponentCollection>::iterator colIt;
            for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
            colIt->ZeroAll();
            }
        }
    }
    else //make sure that when sum is zero, then all factors are 0
    {
        for (size_t i = 0; i < targetRepresentations->size(); i++) {
        scores.push_back(ScoreFactory(0));
        }
    }
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
