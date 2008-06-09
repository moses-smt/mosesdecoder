// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/
#ifdef WIN32
#include <hash_set>
#else
#include <ext/hash_set>
#endif

#include <limits>
#include <cmath>
#include "Manager.h"
#include "TypeDef.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "TrellisPath.h"
#include "TrellisPathCollection.h"
#include "TranslationOption.h"
#include "LMList.h"
#include "TranslationOptionCollection.h"
#include "DummyScoreProducers.h"

using namespace std;

Manager::Manager(InputType const& source, SearchAlgorithm searchAlgorithm)
:m_source(source)
,m_transOptColl(source.CreateTranslationOptionCollection())
,m_search(Search::CreateSearch(source, searchAlgorithm, *m_transOptColl))
,m_start(clock())
,interrupted_flag(0)
{
	VERBOSE(1, "Translating: " << m_source << endl);
	const StaticData &staticData = StaticData::Instance();
	staticData.InitializeBeforeSentenceProcessing(source);
}

Manager::~Manager() 
{
  delete m_transOptColl;
	delete m_search;

	StaticData::Instance().CleanUpAfterSentenceProcessing();      

	clock_t end = clock();
	float et = (end - m_start);
	et /= (float)CLOCKS_PER_SEC;
	VERBOSE(1, "Translation took " << et << " seconds" << endl);
	VERBOSE(1, "Finished translating" << endl);
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void Manager::ProcessSentence()
{
	const StaticData &staticData = StaticData::Instance();
	staticData.ResetSentenceStats(m_source);
	const vector <DecodeGraph*>
			&decodeStepVL = staticData.GetDecodeStepVL();
	
	// create list of all possible translations
	// this is only valid if:
	//		1. generation of source sentence is not done 1st
	//		2. initial hypothesis factors are given in the sentence
	//CreateTranslationOptions(m_source, phraseDictionary, lmListInitial);
	m_transOptColl->CreateTranslationOptions(decodeStepVL);

	m_search->ProcessSentence();
}

/**
 * After decoding, the hypotheses in the stacks and additional arcs
 * form a search graph that can be mined for n-best lists.
 * The heavy lifting is done in the TrellisPath and TrellisPathCollection
 * this function controls this for one sentence.
 *
 * \param count the number of n-best translations to produce
 * \param ret holds the n-best list that was calculated
 */
void Manager::CalcNBest(size_t count, TrellisPathList &ret,bool onlyDistinct) const
{
	if (count <= 0)
		return;

	const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();

	vector<const Hypothesis*> sortedPureHypo = hypoStackColl.back()->GetSortedList();

	if (sortedPureHypo.size() == 0)
		return;

	TrellisPathCollection contenders;

	set<Phrase> distinctHyps;

	// add all pure paths
	vector<const Hypothesis*>::const_iterator iterBestHypo;
	for (iterBestHypo = sortedPureHypo.begin() 
			; iterBestHypo != sortedPureHypo.end()
			; ++iterBestHypo)
	{
		contenders.Add(new TrellisPath(*iterBestHypo));
	}

  // factor defines stopping point for distinct n-best list if too many candidates identical
	size_t nBestFactor = StaticData::Instance().GetNBestFactor();
  if (nBestFactor < 1) nBestFactor = 1000; // 0 = unlimited

	// MAIN loop
	for (size_t iteration = 0 ; (onlyDistinct ? distinctHyps.size() : ret.GetSize()) < count && contenders.GetSize() > 0 && (iteration < count * nBestFactor) ; iteration++)
	{
		// get next best from list of contenders
		TrellisPath *path = contenders.pop();
		assert(path);
		if(onlyDistinct)
		{
			Phrase tgtPhrase = path->GetSurfacePhrase();
			if (distinctHyps.insert(tgtPhrase).second) 
        ret.Add(path);
		}
		else 
    {
		  ret.Add(path);
    }
 
		// create deviations from current best
		path->CreateDeviantPaths(contenders);		

		if(onlyDistinct)
		{
			const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
			if (nBestFactor > 0)
				contenders.Prune(count * nBestFactor);
		}
		else
		{
			contenders.Prune(count);
		}
	}
}

void Manager::CalcDecoderStatistics() const 
{
  const Hypothesis *hypo = GetBestHypothesis();
	if (hypo != NULL)
  {
		StaticData::Instance().GetSentenceStats().CalcFinalStats(*hypo);
    IFVERBOSE(2) {
		 	if (hypo != NULL) {
		   	string buff;
		  	string buff2;
		   	TRACE_ERR( "Source and Target Units:"
		 							<< *StaticData::Instance().GetInput());
				buff2.insert(0,"] ");
				buff2.insert(0,(hypo->GetCurrTargetPhrase()).ToString());
				buff2.insert(0,":");
				buff2.insert(0,(hypo->GetCurrSourceWordsRange()).ToString());
				buff2.insert(0,"[");
				
				hypo = hypo->GetPrevHypo();
				while (hypo != NULL) {
					//dont print out the empty final hypo
				  buff.insert(0,buff2);
				  buff2.clear();
				  buff2.insert(0,"] ");
				  buff2.insert(0,(hypo->GetCurrTargetPhrase()).ToString());
				  buff2.insert(0,":");
				  buff2.insert(0,(hypo->GetCurrSourceWordsRange()).ToString());
				  buff2.insert(0,"[");
				  hypo = hypo->GetPrevHypo();
				}
				TRACE_ERR( buff << endl);
      }
    }
  }
}

void OutputWordGraph(std::ostream &outputWordGraphStream, const Hypothesis *hypo, size_t &linkId)
{
	const StaticData &staticData = StaticData::Instance();

	const Hypothesis *prevHypo = hypo->GetPrevHypo();
			const Phrase *sourcePhrase = hypo->GetSourcePhrase();
			const Phrase &targetPhrase = hypo->GetCurrTargetPhrase();

			
			outputWordGraphStream << "J=" << linkId++
						<< "\tS=" << prevHypo->GetId()
						<< "\tE=" << hypo->GetId()
						<< "\ta=";

			// phrase table scores
			const std::vector<PhraseDictionary*> &phraseTables = staticData.GetPhraseDictionaries();
			std::vector<PhraseDictionary*>::const_iterator iterPhraseTable;
			for (iterPhraseTable = phraseTables.begin() ; iterPhraseTable != phraseTables.end() ; ++iterPhraseTable)
			{
				const PhraseDictionary *phraseTable = *iterPhraseTable;
				vector<float> scores = hypo->GetScoreBreakdown().GetScoresForProducer(phraseTable);

				outputWordGraphStream << scores[0];
				vector<float>::const_iterator iterScore;
				for (iterScore = ++scores.begin() ; iterScore != scores.end() ; ++iterScore)
				{
					outputWordGraphStream << ", " << *iterScore;
				}
			}

			// language model scores
			outputWordGraphStream << "\tl=";
			const LMList &lmList = staticData.GetAllLM();
			LMList::const_iterator iterLM;
			for (iterLM = lmList.begin() ; iterLM != lmList.end() ; ++iterLM)
			{
				LanguageModel *lm = *iterLM;
				vector<float> scores = hypo->GetScoreBreakdown().GetScoresForProducer(lm);
				
				outputWordGraphStream << scores[0];
				vector<float>::const_iterator iterScore;
				for (iterScore = ++scores.begin() ; iterScore != scores.end() ; ++iterScore)
				{
					outputWordGraphStream << ", " << *iterScore;
				}
			}

			// re-ordering
			outputWordGraphStream << "\tr=";

			outputWordGraphStream << hypo->GetScoreBreakdown().GetScoreForProducer(staticData.GetDistortionScoreProducer());

			// lexicalised re-ordering
			const std::vector<LexicalReordering*> &lexOrderings = staticData.GetReorderModels();
			std::vector<LexicalReordering*>::const_iterator iterLexOrdering;
			for (iterLexOrdering = lexOrderings.begin() ; iterLexOrdering != lexOrderings.end() ; ++iterLexOrdering)
			{
				LexicalReordering *lexicalReordering = *iterLexOrdering;
				vector<float> scores = hypo->GetScoreBreakdown().GetScoresForProducer(lexicalReordering);
				
				outputWordGraphStream << scores[0];
				vector<float>::const_iterator iterScore;
				for (iterScore = ++scores.begin() ; iterScore != scores.end() ; ++iterScore)
				{
					outputWordGraphStream << ", " << *iterScore;
				}
			}

			// words !!
			outputWordGraphStream << "\tw=" << hypo->GetCurrTargetPhrase();

			outputWordGraphStream << endl;
}

void Manager::GetWordGraph(long translationId, std::ostream &outputWordGraphStream) const
{
	const StaticData &staticData = StaticData::Instance();
	string fileName = staticData.GetParam("output-word-graph")[0];
	bool outputNBest = Scan<bool>(staticData.GetParam("output-word-graph")[1]);
	const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();

	outputWordGraphStream << "VERSION=1.0" << endl
								<< "UTTERANCE=" << translationId << endl;

	size_t linkId = 0;
	size_t stackNo = 1;
	std::vector < HypothesisStack* >::const_iterator iterStack;
	for (iterStack = ++hypoStackColl.begin() ; iterStack != hypoStackColl.end() ; ++iterStack)
	{
		cerr << endl << stackNo++ << endl;
		const HypothesisStack &stack = **iterStack;
		HypothesisStack::const_iterator iterHypo;
		for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo)
		{
			const Hypothesis *hypo = *iterHypo;
			OutputWordGraph(outputWordGraphStream, hypo, linkId);
			
			if (outputNBest)
			{
				const ArcList *arcList = hypo->GetArcList();
				if (arcList != NULL)
				{
					ArcList::const_iterator iterArcList;
					for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList)
					{
						const Hypothesis *loserHypo = *iterArcList;
						OutputWordGraph(outputWordGraphStream, loserHypo, linkId);
					}
				}
			} //if (outputNBest)
		} //for (iterHypo
	} // for (iterStack 
}

void OutputSearchGraph(long translationId, std::ostream &outputSearchGraphStream, const Hypothesis *hypo, const Hypothesis *recombinationHypo, int forward, double fscore)
{
        outputSearchGraphStream << translationId
				<< " hyp=" << hypo->GetId()
				<< " stack=" << hypo->GetWordsBitmap().GetNumWordsCovered();
	if (hypo->GetId() > 0)
	{
	  const Hypothesis *prevHypo = hypo->GetPrevHypo();
	  outputSearchGraphStream << " back=" << prevHypo->GetId()
				  << " score=" << hypo->GetScore()
				  << " transition=" << (hypo->GetScore() - prevHypo->GetScore());
	}

	if (recombinationHypo != NULL)
	{
	  outputSearchGraphStream << " recombined=" << recombinationHypo->GetId();
	}

	outputSearchGraphStream << " forward=" << forward
				<< " fscore=" << fscore;

	if (hypo->GetId() > 0)
	{
	  outputSearchGraphStream << " covered=" << hypo->GetCurrSourceWordsRange().GetStartPos() 
				  << "-" << hypo->GetCurrSourceWordsRange().GetEndPos()
				  << " out=" << hypo->GetCurrTargetPhrase();
	}

	outputSearchGraphStream << endl;
}

void Manager::GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const
{
  std::map < int, bool > connected;
  std::map < int, int > forward;
  std::map < int, double > forwardScore;

  // *** find connected hypotheses ***

  std::vector< const Hypothesis *> connectedList;

  // start with the ones in the final stack
	const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();
  const HypothesisStack &finalStack = *hypoStackColl.back();
  HypothesisStack::const_iterator iterHypo;
  for (iterHypo = finalStack.begin() ; iterHypo != finalStack.end() ; ++iterHypo)
  {
    const Hypothesis *hypo = *iterHypo;
    connected[ hypo->GetId() ] = true;
    connectedList.push_back( hypo );
  }

  // move back from known connected hypotheses
  for(size_t i=0; i<connectedList.size(); i++) {
    const Hypothesis *hypo = connectedList[i];

    // add back pointer
    const Hypothesis *prevHypo = hypo->GetPrevHypo();
    if (prevHypo->GetId() > 0 // don't add empty hypothesis
	&& connected.find( prevHypo->GetId() ) == connected.end()) // don't add already added
    {
      connected[ prevHypo->GetId() ] = true;
      connectedList.push_back( prevHypo );
    }

    // add arcs
    const ArcList *arcList = hypo->GetArcList();
    if (arcList != NULL)
    {
      ArcList::const_iterator iterArcList;
      for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList)
      {
	const Hypothesis *loserHypo = *iterArcList;
	if (connected.find( loserHypo->GetId() ) == connected.end()) // don't add already added
	{
	  connected[ loserHypo->GetId() ] = true;
	  connectedList.push_back( loserHypo );
	}
      }
    }
  }

  // ** compute best forward path for each hypothesis *** //

  // forward cost of hypotheses on final stack is 0
  for (iterHypo = finalStack.begin() ; iterHypo != finalStack.end() ; ++iterHypo)
  {
    const Hypothesis *hypo = *iterHypo;
    forwardScore[ hypo->GetId() ] = 0.0f;
    forward[ hypo->GetId() ] = -1;
  }

  // compete for best forward score of previous hypothesis
  std::vector < HypothesisStack* >::const_iterator iterStack;
  for (iterStack = --hypoStackColl.end() ; iterStack != hypoStackColl.begin() ; --iterStack)
  {
    const HypothesisStack &stack = **iterStack;
    HypothesisStack::const_iterator iterHypo;
    for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo)
    {
      const Hypothesis *hypo = *iterHypo;
      if (connected.find( hypo->GetId() ) != connected.end())
      {
	// make a play for previous hypothesis
	const Hypothesis *prevHypo = hypo->GetPrevHypo();
	double fscore = forwardScore[ hypo->GetId() ] +
	  hypo->GetScore() - prevHypo->GetScore();
	if (forwardScore.find( prevHypo->GetId() ) == forwardScore.end()
	    || forwardScore.find( prevHypo->GetId() )->second < fscore)
	{
	  forwardScore[ prevHypo->GetId() ] = fscore;
	  forward[ prevHypo->GetId() ] = hypo->GetId();
	}
	// all arcs also make a play
        const ArcList *arcList = hypo->GetArcList();
        if (arcList != NULL)
	{
	  ArcList::const_iterator iterArcList;
	  for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList)
	  {
	    const Hypothesis *loserHypo = *iterArcList;
	    // make a play
	    const Hypothesis *loserPrevHypo = loserHypo->GetPrevHypo();
	    double fscore = forwardScore[ hypo->GetId() ] +
	      loserHypo->GetScore() - loserPrevHypo->GetScore();
	    if (forwardScore.find( loserPrevHypo->GetId() ) == forwardScore.end()
		|| forwardScore.find( loserPrevHypo->GetId() )->second < fscore)
	    {
	      forwardScore[ loserPrevHypo->GetId() ] = fscore;
	      forward[ loserPrevHypo->GetId() ] = loserHypo->GetId();
	    }
	  } // end for arc list  
	} // end if arc list empty
      } // end if hypo connected
    } // end for hypo
  } // end for stack

  // *** output all connected hypotheses *** //
  
  connected[ 0 ] = true;
  for (iterStack = hypoStackColl.begin() ; iterStack != hypoStackColl.end() ; ++iterStack)
  {
    const HypothesisStack &stack = **iterStack;
    HypothesisStack::const_iterator iterHypo;
    for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo)
    {
      const Hypothesis *hypo = *iterHypo;
      if (connected.find( hypo->GetId() ) != connected.end())
      {
	OutputSearchGraph(translationId, outputSearchGraphStream, hypo, NULL, forward[ hypo->GetId() ], forwardScore[ hypo->GetId() ]);
	
	const ArcList *arcList = hypo->GetArcList();
	if (arcList != NULL)
	{
	  ArcList::const_iterator iterArcList;
	  for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList)
	  {
	    const Hypothesis *loserHypo = *iterArcList;
	    OutputSearchGraph(translationId, outputSearchGraphStream, loserHypo, hypo, forward[ hypo->GetId() ], forwardScore[ hypo->GetId() ]);
	  }
	} // end if arcList empty
      } // end if connected
    } // end for iterHypo
  } // end for iterStack 
}

const Hypothesis *Manager::GetBestHypothesis() const
{
	return m_search->GetBestHypothesis();
}
