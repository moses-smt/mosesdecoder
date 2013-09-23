/*
 * ContextFeature.cpp
 *
 *  Created on: Sep 18, 2013
 *      Author: braunefe
 */

#include "moses/FF/ContextFeature.h"
#include "moses/Util.h"
#include "moses/StaticData.h"
#include "moses/RuleMap.h"
#include "moses/Util.h"
#include "moses/SyntaxFeatures/InputTreeRep.h"
#include <vector>
#include <iostream>
#include <fstream>

#include "moses/psd/FeatureExtractor.h"
#include "moses/psd/FeatureConsumer.h"
#include "moses/ChartTranslationOptions.h"
#include "util/check.hh"

using namespace std;
using namespace PSD;

#include <cstdlib>
#include <string>
#include <vector>
#include <iostream>

namespace Moses
{

//override
void ContextFeature::Evaluate(const Phrase &source
	                        , const TargetPhrase &targetPhrase
	                        , ScoreComponentCollection &scoreBreakdown
	                        , ScoreComponentCollection &estimatedFutureScore) const
	{
		targetPhrase.SetRuleSource(source);


	}

void ContextFeature::Evaluate(const InputType &input
	                        , const InputPath &inputPath
	                        , ChartTranslationOptions &transOpts) const
	{

	 vector<FactorType> srcFactors;
	    srcFactors.push_back(0);
	    string nonTermRep = "[X][X]";

	    VERBOSE(5, "Computing vw scores for source context : " << input << endl);

	        //all words ranges should be the same, otherwise crash
	        CHECK(transOpts.GetSourceWordsRange() == inputPath.GetWordsRange());

	        std::string sourceSide = "";
	        std::string targetRepresentation;
	        ChartTranslationOptions::CollType::iterator itr_targets;

	        //map source to accumulated target phrases
	         RuleMap ruleMap;

	         //Map target representation to targetPhrase
	         std::map<std::string,ChartTranslationOption> targetRepMap;

	         VERBOSE(5, "Looping over target phrase collection for recomputing feature scores" << endl);

	        //Get non-rescored target phrase collection for inserting newly scored target
	        for(
	            itr_targets = transOpts.GetTargetPhrases().begin();
	            itr_targets != transOpts.GetTargetPhrases().end();
	            itr_targets++)
	        {
	        	ChartTranslationOption &transOpt = **itr_targets;
	        	const TargetPhrase &tp = transOpt.GetPhrase();
	            //get source side of rule
	            CHECK(tp.GetRuleSource() != NULL);

	            //rewrite non-terminals non source side with "[][]"
	            for(int i=0; i< tp.GetRuleSource()->GetSize();i++)
	            {
	                //replace X by [X][X] for coherence with rule table
	                if(tp.GetRuleSource()->GetWord(i).IsNonTerminal())
	                {
	                    sourceSide += nonTermRep;
	                }
	                else
	                {
	                    sourceSide += tp.GetRuleSource()->GetWord(i).GetString(srcFactors,0);
	                }

	                if(i< tp.GetRuleSource()->GetSize() -1)
	                {
	                    sourceSide += " ";
	                }
	            }

	            //Add parent to source
	            std::string parentNonTerm = "[X]";
	            sourceSide += " ";
	            sourceSide += parentNonTerm;

	            int wordCounter = 0;

	            //NonTermCounter should stay smaller than nonTermIndexMap
	            for(int i=0; i<tp.GetSize();i++)
	            {
	                CHECK(wordCounter < tp.GetSize());

	                //look for non-terminals
	                if(tp.GetWord(i).IsNonTerminal() == 1)
	                {
	                    //append non-terminal
	                    targetRepresentation += nonTermRep;

	                    //Debugging : everything OK in non term index map
	                    //for(int i=0; i < 3; i++)
	                    //{std::cout << "TEST : Non Term index map : " << i << "=" << ntim[i] << std::endl;}

	                    const AlignmentInfo alignInfo = tp.GetAlignNonTerm();
	                    const AlignmentInfo::NonTermIndexMap alignInfoIndex = tp.GetAlignNonTerm().GetNonTermIndexMap();
	                    string alignInd;

	                    //To get the annotation corresponding to the source, iterate through targets and get source representation
	                    AlignmentInfo::const_iterator itr_align;

	                    //Do not use the positions
	                    for(itr_align = alignInfo.begin(); itr_align != alignInfo.end(); itr_align++)
	                    {

	                    	//look for current target
	                    	if( itr_align->second == wordCounter)
	                    	{
	                    		//target annotation is source corresponding to this target
	                    		stringstream s;
	                        	s << alignInfoIndex[itr_align->second];
	                        	alignInd = s.str();
	                    	}
	                    }
	                    targetRepresentation += alignInd;
	                }
	                else
	                {
	                    targetRepresentation += tp.GetWord(i).GetString(srcFactors,0);
	                }
	                if(i<tp.GetSize() -1)
	                {
	                    targetRepresentation += " ";
	                }
	                wordCounter++;
	            }

	            //add parent label to target
	            targetRepresentation += " ";
	            targetRepresentation += parentNonTerm;

	            VERBOSE(5, "STRINGS PUT IN RULE MAP : " << sourceSide << "::" << targetRepresentation << endl);
	            ruleMap.AddRule(sourceSide,targetRepresentation);
	            targetRepMap.insert(std::make_pair(targetRepresentation,transOpt));

	            //clean strings
	            sourceSide = "";
	            targetRepresentation = "";
	        }

	        RuleMap::const_iterator itr_ruleMap;
	        for(itr_ruleMap = ruleMap.begin(); itr_ruleMap != ruleMap.end(); itr_ruleMap++)
	        {
	            VERBOSE(3, "Calling vw for source side : " << itr_ruleMap->first << endl);
	            VERBOSE(3, "Calling vw for source context : " << input << endl);

	            vector<ScoreComponentCollection> scores = ScoreRules(	   //get begin of span
	                                                                       transOpts.GetSourceWordsRange().GetStartPos(),
	                                                                       //get end of span
	                                                                       transOpts.GetSourceWordsRange().GetEndPos(),
	                                                                       //get first of iter rule map
	                                                                       itr_ruleMap->first,
	                                                                       itr_ruleMap->second,
	                                                                       input,
	                                                                       &targetRepMap
	                                                                       );

	            //loop over target phrases and add score
	            std::vector<ScoreComponentCollection>::const_iterator iterLCSP = scores.begin();
	            std::vector<std::string> :: iterator itr_targetRep;
	            for (itr_targetRep = (itr_ruleMap->second)->begin() ; itr_targetRep != (itr_ruleMap->second)->end() ; itr_targetRep++) {
	                CHECK(targetRepMap.find(*itr_targetRep) != targetRepMap.end());

	                //Find target phrase corresponding to representation
	            	const string &str = *itr_targetRep;
	                std::map<std::string,ChartTranslationOption> :: iterator itr_rep;
	                itr_rep = targetRepMap.find(str);

	                VERBOSE(5, "Looking at target phrase : " << itr_rep->second.GetPhrase() << std::endl);
	                //VERBOSE(5, "Target Phrase score vector before adding stateless : ");
	                //StaticData::Instance().GetScoreIndexManager().PrintLabeledScores(std::cerr,(itr_rep->second)->GetScoreBreakdown());
	                // std::cerr << std::endl;
	                //VERBOSE(5, "Target Phrase score before adding stateless : " << (itr_rep->second)->GetPhrase().GetFutureScore() << std::endl);
	                VERBOSE(5, "Score component collection : " << *iterLCSP << std::endl);

	                //How do I put this into target phrase ?
	                const ScoreComponentCollection &scores = *iterLCSP++;
	                ChartTranslationOption &to = itr_rep->second;
	                to.GetScores().PlusEquals(scores);
	                VERBOSE(5, "Target Phrase score after adding stateless : " << (itr_rep->second).GetPhrase().GetFutureScore() << std::endl);
	                }
	        }
	        VERBOSE(5, "Estimate of best score before computing context : " << transOpts.GetEstimateOfBestScore() << std::endl);
	        //transOpts.CalcEstimateOfBestScore();
}


void ContextFeature::Load()
{
	  m_srcFactors.push_back(0);
	  m_tgtFactors.push_back(0);

}

//Was in constructor : add to load method?
//ContextFeature::ContextFeature(ScoreIndexManager &scoreIndexManager, float weight)
//{scoreIndexManager.AddScoreProducer(this);
//vector<float> weights;
//weights.push_back(weight);
//m_srcFactors.push_back(0);
//m_tgtFactors.push_back(0);
//const_cast<StaticData&>(StaticData::Instance()).SetWeightsForScoreProducer(this, weights);
//}

ContextFeature::~ContextFeature()
{
    delete m_extractor;
    delete m_debugExtractor;
    delete m_consumerFactory;
    delete m_debugConsumer;
}

bool ContextFeature::Initialize(const string &modelFile, const string &indexFile, const string &configFile)
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

ScoreComponentCollection ContextFeature::ScoreFactory(float score) const
{
  ScoreComponentCollection out;
  out.Assign(this, score);
  return out;
}

void ContextFeature::CheckIndex(const std::string &targetRep)
{
  if (m_ruleIndex.left.find(targetRep) == m_ruleIndex.left.end())
    throw runtime_error("Phrase not in index: " + targetRep);
}

ChartTranslation ContextFeature::GetPSDTranslation(const string targetRep, const TargetPhrase *tp) const
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
  const AlignmentInfo &alignInfoTerm = tp->GetAlignTerm();
  VERBOSE(5, "Added alignment Info : " << alignInfoTerm << std::endl);
  AlignmentInfo::const_iterator it;
  for (it = alignInfoTerm.begin(); it != alignInfoTerm.end(); it++)
    //cerr << "Added Alignment : " << (*it) << endl;
    psdOpt.m_termAlignment.insert(*it);

  //alignment between non-terminals
  const AlignmentInfo &alignInfoNonTerm = tp->GetAlignNonTerm();
  const vector<size_t> &alignInfoNonTermIndex = tp->GetAlignNonTerm().GetNonTermIndexMap();
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
  const vector<PhraseDictionary*>& ttables = StaticData::Instance().GetPhraseDictionaries();
  const ScoreComponentCollection &scoreCollection = tp->GetScoreBreakdown();
  //std::cerr << "SCORE COMPONENT COLLECTION STORED INTO SCORES : " << scoreCollection << std::endl;

  for(size_t i=0; i< scoreCollection.GetScoresForProducer(ttables[0]).size();i++)
  {
      psdOpt.m_scores.push_back( expl( (long double) scoreCollection.GetScoresForProducer(ttables[0])[i]));
  }

  std::multimap<size_t, size_t>::iterator itr_align;
  return psdOpt;
}

bool ContextFeature::LoadRuleIndex(const string &indexFile)
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


vector<ScoreComponentCollection> ContextFeature::ScoreRules(
                                                                        size_t startSpan,
                                                                        size_t endSpan,
                                                                        const std::string &sourceSide,
                                                                        std::vector<std::string> * targetRepresentations,
                                                                        const InputType &source,
                                                                        map<string,ChartTranslationOption> * targetMap
                                                                      ) const
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

        map<string,ChartTranslationOption> :: iterator itr_rep;
        vector<std::string>::const_iterator tgtRepIt;
        for (tgtRepIt = targetRepresentations->begin(); tgtRepIt != targetRepresentations->end(); tgtRepIt++) {
          CHECK(targetMap->find(*tgtRepIt) != targetMap->end());
          itr_rep = targetMap->find(*tgtRepIt);

          //Can I just get the reference target phrase of this translation option
          psdOptions.push_back(GetPSDTranslation(*tgtRepIt,&(itr_rep->second.GetPhrase())));
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
        //Normalize0(losses);
        Normalize1(losses);
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

void ContextFeature::Normalize0(vector<float> &losses) const
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


void ContextFeature::Normalize1(vector<float> &losses) const
{
  float sum = 0;
  vector<float>::iterator it;
  for (it = losses.begin(); it != losses.end(); it++) {
    *it = exp(-*it);
    sum += *it;
  }
  for (it = losses.begin(); it != losses.end(); it++) {
    *it /= sum;
  }
}


void ContextFeature::Interpolate(vector<float> &losses, vector<float> &pEgivenF, float interpolParam) const
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

void ContextFeature::Normalize(std::vector<float> &losses) const
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

double ContextFeature::LogAddition(double logA, double logB, double logAddPrecision) const
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

} //moses namespace


