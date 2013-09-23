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

			//Question for Hieu : Translation Options contain a collection of target phrses
			//But we also have a targetPhrase here => What to do?

     	    //damt hiero : if we use context features then : For each translation option :
     	        //1. Go through each target phrase and access corresponding source phrase
     	        //2. Store all targetPhrases for this source phrase
     	        //3. Store references to targets in map (TargetRepresentation,TargetPhrase*)
     	        //4. Call vw
     	        //5. Store new score in each targetPhrase
     	        //6. Re-estimate score of this translation option
     	//    #ifdef HAVE_VW

			//iterate over collection
     	   /* std::vector<FactorType> srcFactors;
     	    srcFactors.push_back(0);
     	    std::string nonTermRep = "[X][X]";

     	        std::string sourceSide = "";
     	        std::string targetRepresentation;
     	        TargetPhraseCollection::const_iterator itr_targets;

     	         //Map target representation to targetPhrase
     	         std::map<std::string,TargetPhrase*> targetRepMap;

     	         RuleMap ruleMap;

     	            //get source side of rule
     	            CHECK(tp.GetRuleSource() != NULL);

     	            //rewrite non-terminals non source side with "[][]"
     	            for(int i=0; i<tp.GetRuleSource()->GetSize();i++)
     	            {
     	                //replace X by [X][X] for coherence with rule table
     	                if(tp.GetRuleSource()->GetWord(i).IsNonTerminal())
     	                {
     	                    sourceSide += nonTermRep;
     	                }
     	                else
     	                {
     	                     tp.GetRuleSource()->GetWord(i).GetString(0).AppendToString(&sourceSide);
     	                }

     	                if(i<tp.GetRuleSource()->GetSize() -1)
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
     	            for(int i=0; i<(**itr_targets).GetSize();i++)
     	            {
     	                CHECK(wordCounter < (**itr_targets).GetSize());

     	                //look for non-terminals
     	                if((**itr_targets).GetWord(i).IsNonTerminal() == 1)
     	                {
     	                    //append non-terminal
     	                    targetRepresentation += nonTermRep;

     	                    //Debugging : everything OK in non term index map
     	                    //for(int i=0; i < 3; i++)
     	                    //{std::cout << "TEST : Non Term index map : " << i << "=" << ntim[i] << std::endl;}

     	                    const AlignmentInfo alignInfo = (*itr_targets)->GetAlignNonTerm();
     	                    const AlignmentInfo::NonTermIndexMap alignInfoIndex = (*itr_targets)->GetAlignNonTerm().GetNonTermIndexMap();
     	                    std::string alignInd;

     	                    //To get the annotation corresponding to the source, iterate through targets and get source representation
     	                    AlignmentInfo::const_iterator itr_align;

     	                    //Do not use the positions
     	                    for(itr_align = alignInfo.begin(); itr_align != alignInfo.end(); itr_align++)
     	                    {

     	                    	//look for current target
     	                    	if( itr_align->second == wordCounter)
     	                    	{
     	                    		//target annotation is source corresponding to this target
     	                    		std::stringstream s;
     	                        	s << alignInfoIndex[itr_align->second];
     	                        	alignInd = s.str();
     	                    	}
     	                    }
     	                    targetRepresentation += alignInd;
     	                }
     	                else
     	                {
     	                    targetRepresentation += (**itr_targets).GetWord(i).GetString(srcFactors,0);
     	                }
     	                if(i<(**itr_targets).GetSize() -1)
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
     	            targetRepMap.insert(std::make_pair(targetRepresentation,*itr_targets));
     	            targetRepMap.insert(std::make_pair(targetRepresentation,*itr_targets));

     	            //clean strings
     	            sourceSide = "";
     	            targetRepresentation = "";
     	        }
     	        */

	//Add fake vector

	  //Put target phrases into rule map and score
	  //1. Put strings into rule map
	  //2. Score strings -> vector of losses
	  //3. For each string look for target phrase
	  //4. Associate score to target phrase

	  //loop over target phrases and add score
	   std::vector<ScoreComponentCollection> scores;
	   std::vector<ScoreComponentCollection>::const_iterator iterLCSP = scores.begin();
	   std::vector<std::string> :: iterator itr_targetRep;

	   for (itr_targetRep = (itr_ruleMap->second)->begin() ; itr_targetRep != (itr_ruleMap->second)->end() ; itr_targetRep++) {
	                //Find target phrase corresponding to representation
	                std::map<std::string,TargetPhrase*> :: iterator itr_rep;
	                CHECK(targetRepMap.find(*itr_targetRep) != targetRepMap.end());
	                itr_rep = targetRepMap.find(*itr_targetRep);
	                VERBOSE(5, "Looking at target phrase : " << *itr_rep->second << std::endl);
	                //VERBOSE(5, "Target Phrase score vector before adding stateless : ");
	                //StaticData::Instance().GetScoreIndexManager().PrintLabeledScores(std::cerr,(itr_rep->second)->GetScoreBreakdown());
	                // std::cerr << std::endl;
	                VERBOSE(5, "Target Phrase score before adding stateless : " << (itr_rep->second)->GetFutureScore() << std::endl);
	                VERBOSE(5, "Score component collection : " << *iterLCSP << std::endl);
	                (itr_rep->second)->AddStatelessScore(*iterLCSP++);
	                VERBOSE(5, "Target Phrase score after adding stateless : " << (itr_rep->second)->GetFutureScore() << std::endl);
	                }


	}



}


