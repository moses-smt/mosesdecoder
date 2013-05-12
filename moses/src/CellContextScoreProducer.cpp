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

  bool isGood = false;

  m_consumerFactory = new VWLibraryPredictConsumerFactory(modelFile, 255);
  if (! LoadRuleIndex(indexFile))
  isGood = false;

  m_extractorConfig.Load(configFile);
  m_extractor = new FeatureExtractor(m_ruleIndex,m_extractorConfig, false);
  m_debugExtractor = new FeatureExtractor(m_ruleIndex,m_extractorConfig, true);
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
  //std::cerr << "SCORE COMPONENT COLLECTION STORED INTO SCORES : " << scoreCollection << std::endl;

  for(size_t i=0; i< scoreCollection.GetScoresForProducer(ttables[0]).size();i++)
  {
      psdOpt.m_scores.push_back( expl( (long double) scoreCollection.GetScoresForProducer(ttables[0])[i]));
  }

  std::multimap<size_t, size_t>::iterator itr_align;
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

          psdOptions.push_back(GetPSDTranslation(*tgtRepIt,itr_rep->second));
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
        VERBOSE(5, "VW losses BEFORE interpolation : " << std::endl);
        vector<float>::iterator lossIt;
        for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
           VERBOSE(5, *lossIt << " ");}
        Interpolate(losses,pEgivenF,0.1);
        //m_debugExtractor->GenerateFeaturesChart(m_debugConsumer,source.m_PSDContext,sourceSide,syntFeats,parentLabel.GetString(),span,startSpan,endSpan,psdOptions,losses,pEgivenF);
        //Normalize(losses);
        VERBOSE(5, "VW losses after interpolation : " << std::endl);
        for (lossIt = losses.begin(); lossIt != losses.end(); lossIt++) {
        VERBOSE(5, *lossIt << " ");
        float logScore = PreciseEquals( (long double) *lossIt, 0.0) ? LOWEST_SCORE : log(*lossIt);
        *lossIt = logScore;
        VERBOSE(5, "Interpolated loss : " << *lossIt << " ");
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
		 VERBOSE(5, "Loss before interpolation : " <<  *itLosses  << endl);
		 VERBOSE(5, "Interpol param : " <<  interpolParam  << " : " << 1.0 - interpolParam << endl);
		 VERBOSE(5, "E given F : " <<  *itEgivenF << endl);
		 *itLosses = (interpolParam*(*itEgivenF)) + ( (1.0 - interpolParam) * (*itLosses) );
		 //*itLosses = LogAddition(interpolParam*(*itEgivenF) , ( (1.0 - interpolParam) * (log(*itLosses))) , 5.0);
		 VERBOSE(5, "Loss after interpolation : " <<  *itLosses  << endl);
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
}

double CellContextScoreProducer::LogAddition(double logA, double logB, double logAddPrecision)
{

	if (logA == logB) {
		return (logA+log(2.0));}

	  if (logA > logB) {
	    if (logA - logB > logAddPrecision) {
	        return(logA);
	    }
	    else {
	        return(logA + log(1.0 + pow(2.718,logB - logA)));
	    }
	  }

	  if (logB - logA > logAddPrecision) {
	        return(logB);
	  }

	  return(logB + log(1 + pow(2.718,logA - logB)));
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
