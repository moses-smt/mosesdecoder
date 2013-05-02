// $Id: CellContextScoreProducer.cpp,v 1.2 2012/10/08 14:52:03 braunefe Exp $

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
    delete m_debugExtractor;
    delete m_consumerFactory;
    delete m_debugConsumer;
}

bool CellContextScoreProducer::Initialize(const string &modelFile, const string &indexFile, const string &configFile)
{
  bool isGood = LoadRuleIndex(indexFile);

  m_consumerFactory = new VWLibraryPredictConsumerFactory(modelFile, 255);
  if (! LoadRuleIndex(indexFile))
  isGood = false;

  m_extractorConfig.Load(configFile);

  m_extractor = new FeatureExtractor(m_ruleIndex, m_extractorConfig, false);
  m_debugExtractor = new FeatureExtractor(m_ruleIndex, m_extractorConfig, true);
  m_debugConsumer = new VWFileTrainConsumer("/projekte/morphosynt/braune/MyPerlScripts/softSyntax/hiero-feature-debug");
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

bool CellContextScoreProducer::CheckIndex(const std::string &targetRep)
{
  bool returnValue = true;
  if (m_ruleIndex.left.find(targetRep) == m_ruleIndex.left.end())
  { std::cerr << "POTENTIAL ERROR : Phrase not in index: " + targetRep << std::endl;
    returnValue = false;
  }
	  //throw runtime_error("Phrase not in index: " + targetRep);
  return returnValue;
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
  const vector<size_t> &alignInfoNonTermIndex = tp->GetAlignmentInfo().GetNonTermIndexMap();
  VERBOSE(5, "Added alignment Info between non terms : " << alignInfoNonTerm << std::endl);
  //TODO : modify implementation here : count the number of non-terminals and search
  //Do not use the positions
  for (it = alignInfoNonTerm.begin(); it != alignInfoNonTerm.end(); it++)
  {
	  std:pair<size_t,size_t> nonTermIndexMapPair = std::make_pair(it->second,alignInfoNonTermIndex[it->second]);
	  VERBOSE(5,std::cerr << "Inserted alignment : " <<  it->second << ": " << alignInfoNonTermIndex[it->second] << std::endl);
	  psdOpt.m_nonTermAlignment.insert(nonTermIndexMapPair);
  }

  // scores
  const TranslationSystem& system = StaticData::Instance().GetTranslationSystem(TranslationSystem::DEFAULT);
  const vector<PhraseDictionaryFeature*>& ttables = system.GetPhraseDictionaries();
  const ScoreComponentCollection &scoreCollection = tp->GetScoreBreakdown();
  psdOpt.m_scores = scoreCollection.GetScoresForProducer(ttables[0]); // assuming one translation step!
  for(size_t i=0; i<psdOpt.m_scores.size();i++)
  {
      psdOpt.m_scores[i] = exp(psdOpt.m_scores[i]);
  }

  std::multimap<size_t, size_t>::iterator itr_align;
  /*std::cerr << "Created Translation Option : ";
  for(itr_align = psdOpt.m_nonTermAlignment.begin(); itr_align != psdOpt.m_nonTermAlignment.end(); itr_align++)
  {
	  std::cerr << itr_align->first << " : " <<  itr_align->second << "\t";
  }
  std::cerr << std::endl;*/

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
    string span;

    //get span
    int spanSize = (endSpan-startSpan) + 1;
    stringstream s;
    s << spanSize;
    span = s.str();

    if(targetRepresentations->size() > 1)
    {

        vector<float> losses(targetRepresentations->size());

        //Fabienne Braune : vector of pEgivenF for interpolation
        vector<float> pEgivenF(targetRepresentations->size());
        vector<ChartTranslation> psdOptions;

        map<string,TargetPhrase*> :: iterator itr_rep;
        vector<std::string>::const_iterator tgtRepIt;
        for (tgtRepIt = targetRepresentations->begin(); tgtRepIt != targetRepresentations->end(); tgtRepIt++) {
          CHECK(targetMap->find(*tgtRepIt) != targetMap->end());
          itr_rep = targetMap->find(*tgtRepIt);
          VERBOSE(5, "CHECKING INDEX FOR : " << *tgtRepIt << endl);

          bool DoesIndexExist = CheckIndex(*tgtRepIt);
          if(DoesIndexExist)
          {psdOptions.push_back(GetPSDTranslation(*tgtRepIt,itr_rep->second));}
        }

        VERBOSE(5, "Extracting features for source : " << sourceSide << endl);
        VERBOSE(5, "Extracting features for spans : " << startSpan << " : " << endSpan << endl);

        //std::cerr << "GETTING SYNTAX LABELS" << std::endl;
        bool IsBegin = false;
        //skip first symbol in sentence which is <s>
        vector<SyntaxLabel> syntaxLabels = source.GetInputTreeRep()->GetLabels(startSpan-1,endSpan-1);
        SyntaxLabel parentLabel = source.GetInputTreeRep()->GetParent(startSpan-1,endSpan-1,IsBegin);
        vector<string> syntFeats;

        //damt hiero : TODO : use GetNoTag : also in extract-syntax features
        string noTag = "NOTAG";
        IsBegin = false;
        while(!parentLabel.GetString().compare("NOTAG"))
        {
            parentLabel = source.GetInputTreeRep()->GetParent(startSpan,endSpan,IsBegin);
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
        //std::cerr << "LOOKING FOR SPAN : " << startSpan << " : " << endSpan << std::endl;
        m_extractor->GenerateFeaturesChart(p_consumer,source.m_PSDContext,sourceSide,syntFeats,parentLabel.GetString(),span,startSpan,endSpan,psdOptions,losses,pEgivenF);
        m_consumerFactory->Release(p_consumer);
        Normalize0(losses);
        Interpolate(losses,pEgivenF,0.1);
        //m_debugExtractor->GenerateFeaturesChart(m_debugConsumer,source.m_PSDContext,sourceSide,syntFeats,parentLabel.GetString(),span,startSpan,endSpan,psdOptions,losses);
        //Normalize(losses);
        vector<float>::iterator lossIt;
        VERBOSE(5, "VW losses after normalization : " << std::endl);
        for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
        VERBOSE(5, *lossIt << " ");
        float logScore = Equals(*lossIt, 0) ? LOWEST_SCORE : log(*lossIt);
        *lossIt = logScore;
        //FB : maybe we should floor log score before adding to scores
        //Remove when making example
        //FloorScore(logScore);
        scores.push_back(ScoreFactory(logScore));
        VERBOSE(5, std::endl;);
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

void CellContextScoreProducer::Normalize0(vector<float> &losses)
{
    float sum = 0;

    VERBOSE(3, "VW losses before normalization : " << std::endl);

    // clip to [0,1] and take 1-Z as non-normalized prob
    vector<float>::iterator it;
    for (it = losses.begin(); it != losses.end(); it++) {
      VERBOSE(3, "" <<  *it << " ");
      //clip and 1-Z at the same time
      if (*it <= 0.0) {
    	  //VERBOSE(3, "Smaller or equal 0, putting to " << *it << std::endl);
    	  	  *it = 1.0;
      }
      else {
    	  	  if (*it >= 1.0) {
    	  		  *it = 0.0;
    	  		  //VERBOSE(3, "Greater or equal 1, putting to " << *it << std::endl);
    	  	  }
    	  	  else
    	  	  {
    	  		 //VERBOSE(3, "Between 0 and 1, taking 1 - " << *it << std::endl);
    	  		  *it = 1 - *it;
    	  		//VERBOSE(3, "= " << *it << std::endl);
    	  	  }
           }
      sum += *it;
      //VERBOSE(3, "My sum is " << sum << std::endl);
    }
    if (! Equals(sum, 0)) {
    	//VERBOSE(3, "Sum not equals 0 . Divide each loss by sum" << std::endl);
      // normalize
      for (it = losses.begin(); it != losses.end(); it++)
    	  //VERBOSE(3, "Dividing " << *it << " by " << sum << std::endl);
	*it /= sum;
		  //VERBOSE(3, "Result of division " << *it << std::endl);
    } else {
    	//VERBOSE(3, "Sum equals 0 : make uniform");
      // sum of non-normalized probs is 0, then take uniform probs
      for (it = losses.begin(); it != losses.end(); it++) 
	{*it = 1.0 / losses.size();}
      //VERBOSE(3, "Created uniform proba : " << *it << " ");
    }
    VERBOSE(3, std::endl;);
}


void CellContextScoreProducer::Interpolate(vector<float> &losses, vector<float> &pEgivenF, float interpolParam)
{
	vector<float>::iterator lossIt;
	vector<float>::iterator pEgivenFit;

	CHECK(losses.size() == pEgivenF.size());

	 vector<float>::iterator itLosses;
	 vector<float>::iterator itEgivenF;

	 for (itLosses = losses.begin(), itEgivenF = pEgivenF.begin(); itLosses != losses.end(), itEgivenF != pEgivenF.end(); itLosses++,itEgivenF++) {
		 *itLosses = interpolParam*(*itEgivenF) + ( (1.0 - interpolParam) * (*itLosses));
	    }
}

void CellContextScoreProducer::Normalize(std::vector<float> &losses)
{
    //Normalization
      vector<float>::iterator lossIt;
      float sum = 0;
      for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
      *lossIt = exp(-*lossIt);
      VERBOSE(4, "Obtained score : " <<  *lossIt  << endl);
      sum += *lossIt;
      VERBOSE(4, "Sum to normalize" << sum << std::endl);
      }
      for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
          *lossIt /= sum;
        }

      // handle case where sum is 0
      /*if (sum != 0) {
          vector<ScoreComponentCollection>::iterator colIt;
          for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
          VERBOSE(5, "Score before normalizing : " << *colIt << std::endl);
          colIt->Assign(this, log(colIt->GetScoreForProducer(this) / sum));
          VERBOSE(5, "Score after normalizing : " << *colIt << std::endl);
          }
          else
        {
            VERBOSE(5, "SUM IS ZERO : SCORES PUT TO 0 " << std::endl);
            vector<ScoreComponentCollection>::iterator colIt;
            for (colIt = scores.begin(); colIt != scores.end(); colIt++) {
            colIt->ZeroAll();
            }
        }
      }*/

}

size_t CellContextScoreProducer::GetNumScoreComponents() const
{
  return 1;
}

std::string CellContextScoreProducer::GetScoreProducerDescription(unsigned) const
{
  return "PSD";
}

std::string CellContextScoreProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "psd";
}

size_t CellContextScoreProducer::GetNumInputScores() const
{
  return 0;
}

} // namespace Moses
