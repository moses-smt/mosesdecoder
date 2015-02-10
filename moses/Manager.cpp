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

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include "Manager.h"
#include "TypeDef.h"
#include "Util.h"
#include "TargetPhrase.h"
#include "TrellisPath.h"
#include "TrellisPathCollection.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "Timer.h"
#include "moses/OutputCollector.h"
#include "moses/FF/DistortionScoreProducer.h"
#include "moses/LM/Base.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/TranslationAnalysis.h"
#include "moses/HypergraphOutput.h"
#include "moses/mbr.h"
#include "moses/LatticeMBR.h"

#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#include "rule.pb.h"
#endif

#include "util/exception.hh"

using namespace std;

namespace Moses
{
Manager::Manager(InputType const& source)
  :BaseManager(source)
  ,m_transOptColl(source.CreateTranslationOptionCollection())
  ,interrupted_flag(0)
  ,m_hypoId(0)
{
  const StaticData &staticData = StaticData::Instance();
  SearchAlgorithm searchAlgorithm = staticData.GetSearchAlgorithm();
  m_search = Search::CreateSearch(*this, source, searchAlgorithm, *m_transOptColl);

  StaticData::Instance().InitializeForInput(m_source);
}

Manager::~Manager()
{
  delete m_transOptColl;
  delete m_search;
  // this is a comment ...

  StaticData::Instance().CleanUpAfterSentenceProcessing(m_source);
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void Manager::Decode()
{
  // initialize statistics
  ResetSentenceStats(m_source);
  IFVERBOSE(2) {
    GetSentenceStats().StartTimeTotal();
  }

  // check if alternate weight setting is used
  // this is not thread safe! it changes StaticData
  if (StaticData::Instance().GetHasAlternateWeightSettings()) {
    if (m_source.GetSpecifiesWeightSetting()) {
      StaticData::Instance().SetWeightSetting(m_source.GetWeightSetting());
    } else {
      StaticData::Instance().SetWeightSetting("default");
    }
  }

  // get translation options
  IFVERBOSE(1) {
    GetSentenceStats().StartTimeCollectOpts();
  }
  m_transOptColl->CreateTranslationOptions();

  // some reporting on how long this took
  IFVERBOSE(1) {
    GetSentenceStats().StopTimeCollectOpts();
    TRACE_ERR("Line "<< m_source.GetTranslationId() << ": Collecting options took "
              << GetSentenceStats().GetTimeCollectOpts() << " seconds at "
              << __FILE__ << ":" << __LINE__ << endl);
  }

  // search for best translation with the specified algorithm
  Timer searchTime;
  searchTime.start();
  m_search->Decode();
  VERBOSE(1, "Line " << m_source.GetTranslationId() << ": Search took " << searchTime << " seconds" << endl);
  IFVERBOSE(2) {
    GetSentenceStats().StopTimeTotal();
    TRACE_ERR(GetSentenceStats());
  }
}

/**
 * Print all derivations in search graph. Note: The number of derivations is exponential in the sentence length
 *
 */

void Manager::PrintAllDerivations(long translationId, ostream& outputStream) const
{
  const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();

  vector<const Hypothesis*> sortedPureHypo = hypoStackColl.back()->GetSortedList();

  if (sortedPureHypo.size() == 0)
    return;

  float remainingScore = 0;
  vector<const TargetPhrase*> remainingPhrases;

  // add all pure paths
  vector<const Hypothesis*>::const_iterator iterBestHypo;
  for (iterBestHypo = sortedPureHypo.begin()
                      ; iterBestHypo != sortedPureHypo.end()
       ; ++iterBestHypo) {
    printThisHypothesis(translationId, *iterBestHypo, remainingPhrases, remainingScore, outputStream);
    printDivergentHypothesis(translationId, *iterBestHypo, remainingPhrases, remainingScore, outputStream);
  }
}

const TranslationOptionCollection* Manager::getSntTranslationOptions()
{
  return m_transOptColl;
}

void Manager::printDivergentHypothesis(long translationId, const Hypothesis* hypo, const vector <const TargetPhrase*> & remainingPhrases, float remainingScore , ostream& outputStream ) const
{
  //Backtrack from the predecessor
  if (hypo->GetId()  > 0) {
    vector <const TargetPhrase*> followingPhrases;
    followingPhrases.push_back(& (hypo->GetCurrTargetPhrase()));
    ///((Phrase) hypo->GetPrevHypo()->GetTargetPhrase());
    followingPhrases.insert(followingPhrases.end()--, remainingPhrases.begin(), remainingPhrases.end());
    printDivergentHypothesis(translationId, hypo->GetPrevHypo(), followingPhrases , remainingScore + hypo->GetScore() - hypo->GetPrevHypo()->GetScore(), outputStream);
  }

  //Process the arcs
  const ArcList *pAL = hypo->GetArcList();
  if (pAL) {
    const ArcList &arcList = *pAL;
    // every possible Arc to replace this edge
    ArcList::const_iterator iterArc;
    for (iterArc = arcList.begin() ; iterArc != arcList.end() ; ++iterArc) {
      const Hypothesis *loserHypo = *iterArc;
      const Hypothesis* loserPrevHypo = loserHypo->GetPrevHypo();
      float arcScore = loserHypo->GetScore() - loserPrevHypo->GetScore();
      vector <const TargetPhrase* > followingPhrases;
      followingPhrases.push_back(&(loserHypo->GetCurrTargetPhrase()));
      followingPhrases.insert(followingPhrases.end()--, remainingPhrases.begin(), remainingPhrases.end());
      printThisHypothesis(translationId, loserPrevHypo, followingPhrases, remainingScore + arcScore, outputStream);
      printDivergentHypothesis(translationId, loserPrevHypo, followingPhrases, remainingScore + arcScore, outputStream);
    }
  }
}


void
Manager::
printThisHypothesis(long translationId, const Hypothesis* hypo,
                    const vector <const TargetPhrase*> & remainingPhrases,
                    float remainingScore, ostream& outputStream) const
{

  outputStream << translationId << " ||| ";

  //Yield of this hypothesis
  hypo->ToStream(outputStream);
  for (size_t p = 0; p < remainingPhrases.size(); ++p) {
    const TargetPhrase * phrase = remainingPhrases[p];
    size_t size = phrase->GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase->GetFactor(pos, 0);
      outputStream << *factor;
      outputStream << " ";
    }
  }

  outputStream << "||| " << hypo->GetScore() + remainingScore;
  outputStream << endl;
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
       ; ++iterBestHypo) {
    contenders.Add(new TrellisPath(*iterBestHypo));
  }

  // factor defines stopping point for distinct n-best list if too many candidates identical
  size_t nBestFactor = StaticData::Instance().GetNBestFactor();
  if (nBestFactor < 1) nBestFactor = 1000; // 0 = unlimited

  // MAIN loop
  for (size_t iteration = 0 ; (onlyDistinct ? distinctHyps.size() : ret.GetSize()) < count && contenders.GetSize() > 0 && (iteration < count * nBestFactor) ; iteration++) {
    // get next best from list of contenders
    TrellisPath *path = contenders.pop();
    UTIL_THROW_IF2(path == NULL, "path is NULL");
    // create deviations from current best
    path->CreateDeviantPaths(contenders);
    if(onlyDistinct) {
      Phrase tgtPhrase = path->GetSurfacePhrase();
      if (distinctHyps.insert(tgtPhrase).second) {
        ret.Add(path);
      } else {
        delete path;
        path = NULL;
      }
    } else {
      ret.Add(path);
    }


    if(onlyDistinct) {
      const size_t nBestFactor = StaticData::Instance().GetNBestFactor();
      if (nBestFactor > 0)
        contenders.Prune(count * nBestFactor);
    } else {
      contenders.Prune(count);
    }
  }
}

struct SGNReverseCompare {
  bool operator() (const SearchGraphNode& s1, const SearchGraphNode& s2) const {
    return s1.hypo->GetId() > s2.hypo->GetId();
  }
};

/**
  * Implements lattice sampling, as in Chatterjee & Cancedda, emnlp 2010
  **/
void Manager::CalcLatticeSamples(size_t count, TrellisPathList &ret) const
{

  vector<SearchGraphNode> searchGraph;
  GetSearchGraph(searchGraph);

  //Calculation of the sigmas of each hypothesis and edge. In C&C notation this is
  //the "log of the cumulative unnormalized probability of all the paths in the
  // lattice for the hypothesis to a final node"
  typedef pair<int, int> Edge;
  map<const Hypothesis*, float> sigmas;
  map<Edge, float> edgeScores;
  map<const Hypothesis*, set<const Hypothesis*> > outgoingHyps;
  map<int,const Hypothesis*> idToHyp;
  map<int,float> fscores;

  //Iterating through the hypos in reverse order of id gives  a reverse
  //topological order. We rely on the fact that hypo ids are given out
  //sequentially, as the search proceeds.
  //NB: Could just sort by stack.
  sort(searchGraph.begin(), searchGraph.end(), SGNReverseCompare());

  //first task is to fill in the outgoing hypos and edge scores.
  for (vector<SearchGraphNode>::const_iterator i = searchGraph.begin();
       i != searchGraph.end(); ++i) {
    const Hypothesis* hypo = i->hypo;
    idToHyp[hypo->GetId()] = hypo;
    fscores[hypo->GetId()] = i->fscore;
    if (hypo->GetId()) {
      //back to  current
      const Hypothesis* prevHypo = i->hypo->GetPrevHypo();
      outgoingHyps[prevHypo].insert(hypo);
      edgeScores[Edge(prevHypo->GetId(),hypo->GetId())] =
        hypo->GetScore() - prevHypo->GetScore();
    }
    //forward from current
    if (i->forward >= 0) {
      map<int,const Hypothesis*>::const_iterator idToHypIter = idToHyp.find(i->forward);
      UTIL_THROW_IF2(idToHypIter == idToHyp.end(),
                     "Couldn't find hypothesis " << i->forward);
      const Hypothesis* nextHypo = idToHypIter->second;
      outgoingHyps[hypo].insert(nextHypo);
      map<int,float>::const_iterator fscoreIter = fscores.find(nextHypo->GetId());
      UTIL_THROW_IF2(fscoreIter == fscores.end(),
                     "Couldn't find scores for hypothsis " << nextHypo->GetId());
      edgeScores[Edge(hypo->GetId(),nextHypo->GetId())] =
        i->fscore - fscoreIter->second;
    }
  }


  //then run through again to calculate sigmas
  for (vector<SearchGraphNode>::const_iterator i = searchGraph.begin();
       i != searchGraph.end(); ++i) {

    if (i->forward == -1) {
      sigmas[i->hypo] = 0;
    } else {
      map<const Hypothesis*, set<const Hypothesis*> >::const_iterator outIter =
        outgoingHyps.find(i->hypo);

      UTIL_THROW_IF2(outIter == outgoingHyps.end(),
                     "Couldn't find hypothesis " << i->hypo->GetId());
      float sigma = 0;
      for (set<const Hypothesis*>::const_iterator j = outIter->second.begin();
           j != outIter->second.end(); ++j) {
        map<const Hypothesis*, float>::const_iterator succIter = sigmas.find(*j);
        UTIL_THROW_IF2(succIter == sigmas.end(),
                       "Couldn't find hypothesis " << (*j)->GetId());
        map<Edge,float>::const_iterator edgeScoreIter =
          edgeScores.find(Edge(i->hypo->GetId(),(*j)->GetId()));
        UTIL_THROW_IF2(edgeScoreIter == edgeScores.end(),
                       "Couldn't find edge for hypothesis " << (*j)->GetId());
        float term = edgeScoreIter->second + succIter->second; // Add sigma(*j)
        if (sigma == 0) {
          sigma = term;
        } else {
          sigma = log_sum(sigma,term);
        }
      }
      sigmas[i->hypo] = sigma;
    }
  }

  //The actual sampling!
  const Hypothesis* startHypo = searchGraph.back().hypo;
  UTIL_THROW_IF2(startHypo->GetId() != 0, "Expecting the start hypothesis ");
  for (size_t i = 0; i < count; ++i) {
    vector<const Hypothesis*> path;
    path.push_back(startHypo);
    while(1) {
      map<const Hypothesis*, set<const Hypothesis*> >::const_iterator outIter =
        outgoingHyps.find(path.back());
      if (outIter == outgoingHyps.end() || !outIter->second.size()) {
        //end of the path
        break;
      }
      //score the possibles
      vector<const Hypothesis*> candidates;
      vector<float> candidateScores;
      float scoreTotal = 0;
      for (set<const Hypothesis*>::const_iterator j = outIter->second.begin();
           j != outIter->second.end(); ++j) {
        candidates.push_back(*j);
        UTIL_THROW_IF2(sigmas.find(*j) == sigmas.end(),
                       "Hypothesis " << (*j)->GetId() << " not found");
        Edge edge(path.back()->GetId(),(*j)->GetId());
        UTIL_THROW_IF2(edgeScores.find(edge) == edgeScores.end(),
                       "Edge not found");
        candidateScores.push_back(sigmas[*j]  + edgeScores[edge]);
        if (scoreTotal == 0) {
          scoreTotal = candidateScores.back();
        } else {
          scoreTotal = log_sum(candidateScores.back(), scoreTotal);
        }
      }

      //normalise
      transform(candidateScores.begin(), candidateScores.end(), candidateScores.begin(), bind2nd(minus<float>(),scoreTotal));
      //copy(candidateScores.begin(),candidateScores.end(),ostream_iterator<float>(cerr," "));
      //cerr << endl;

      //draw the sample
      float frandom = log((float)rand()/RAND_MAX);
      size_t position = 1;
      float sum = candidateScores[0];
      for (; position < candidateScores.size() && sum < frandom; ++position) {
        sum = log_sum(sum,candidateScores[position]);
      }
      //cerr << "Random: " << frandom << " Chose " << position-1 << endl;
      const Hypothesis* chosen =  candidates[position-1];
      path.push_back(chosen);
    }
    //cerr << "Path: " << endl;
    //for (size_t j = 0; j < path.size(); ++j) {
    // cerr << path[j]->GetId() <<  " " << path[j]->GetScoreBreakdown() << endl;
    //}
    //cerr << endl;

    //Convert the hypos to TrellisPath
    ret.Add(new TrellisPath(path));
    //cerr << ret.at(ret.GetSize()-1).GetScoreBreakdown() << endl;
  }

}



void Manager::CalcDecoderStatistics() const
{
  const Hypothesis *hypo = GetBestHypothesis();
  if (hypo != NULL) {
    GetSentenceStats().CalcFinalStats(*hypo);
    IFVERBOSE(2) {
      if (hypo != NULL) {
        string buff;
        string buff2;
        TRACE_ERR( "Source and Target Units:"
                   << hypo->GetInput());
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

void Manager::OutputWordGraph(std::ostream &outputWordGraphStream, const Hypothesis *hypo, size_t &linkId) const
{

  const Hypothesis *prevHypo = hypo->GetPrevHypo();


  outputWordGraphStream << "J=" << linkId++
                        << "\tS=" << prevHypo->GetId()
                        << "\tE=" << hypo->GetId()
                        << "\ta=";

  // phrase table scores
  const std::vector<PhraseDictionary*> &phraseTables = PhraseDictionary::GetColl();
  std::vector<PhraseDictionary*>::const_iterator iterPhraseTable;
  for (iterPhraseTable = phraseTables.begin() ; iterPhraseTable != phraseTables.end() ; ++iterPhraseTable) {
    const PhraseDictionary *phraseTable = *iterPhraseTable;
    vector<float> scores = hypo->GetScoreBreakdown().GetScoresForProducer(phraseTable);

    outputWordGraphStream << scores[0];
    vector<float>::const_iterator iterScore;
    for (iterScore = ++scores.begin() ; iterScore != scores.end() ; ++iterScore) {
      outputWordGraphStream << ", " << *iterScore;
    }
  }

  // language model scores
  outputWordGraphStream << "\tl=";

  const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (size_t i = 0; i < statefulFFs.size(); ++i) {
    const StatefulFeatureFunction *ff = statefulFFs[i];
    const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff);

    vector<float> scores = hypo->GetScoreBreakdown().GetScoresForProducer(lm);

    outputWordGraphStream << scores[0];
    vector<float>::const_iterator iterScore;
    for (iterScore = ++scores.begin() ; iterScore != scores.end() ; ++iterScore) {
      outputWordGraphStream << ", " << *iterScore;
    }
  }

  // re-ordering
  outputWordGraphStream << "\tr=";

  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  std::vector<FeatureFunction*>::const_iterator iter;
  for (iter = ffs.begin(); iter != ffs.end(); ++iter) {
    const FeatureFunction *ff = *iter;

    const DistortionScoreProducer *model = dynamic_cast<const DistortionScoreProducer*>(ff);
    if (model) {
      outputWordGraphStream << hypo->GetScoreBreakdown().GetScoreForProducer(model);
    }
  }

  // lexicalised re-ordering
  /*
  const std::vector<LexicalReordering*> &lexOrderings = StaticData::Instance().GetReorderModels();
  std::vector<LexicalReordering*>::const_iterator iterLexOrdering;
  for (iterLexOrdering = lexOrderings.begin() ; iterLexOrdering != lexOrderings.end() ; ++iterLexOrdering) {
    LexicalReordering *lexicalReordering = *iterLexOrdering;
    vector<float> scores = hypo->GetScoreBreakdown().GetScoresForProducer(lexicalReordering);

    outputWordGraphStream << scores[0];
    vector<float>::const_iterator iterScore;
    for (iterScore = ++scores.begin() ; iterScore != scores.end() ; ++iterScore) {
      outputWordGraphStream << ", " << *iterScore;
    }
  }
  */
  // words !!
//  outputWordGraphStream << "\tw=" << hypo->GetCurrTargetPhrase();

  // output both source and target phrases in the word graph
  outputWordGraphStream << "\tw=" << hypo->GetSourcePhraseStringRep() << "|" << hypo->GetCurrTargetPhrase();

  outputWordGraphStream << endl;
}

void Manager::GetOutputLanguageModelOrder( std::ostream &out, const Hypothesis *hypo ) const
{
  Phrase translation;
  hypo->GetOutputPhrase(translation);
  const std::vector<const StatefulFeatureFunction*> &statefulFFs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (size_t i = 0; i < statefulFFs.size(); ++i) {
    const StatefulFeatureFunction *ff = statefulFFs[i];
    if (const LanguageModel *lm = dynamic_cast<const LanguageModel*>(ff)) {
      lm->ReportHistoryOrder(out, translation);
    }
  }
}

void Manager::GetWordGraph(long translationId, std::ostream &outputWordGraphStream) const
{
  const StaticData &staticData = StaticData::Instance();
  const PARAM_VEC *params;

  string fileName;
  bool outputNBest = false;
  params = staticData.GetParameter().GetParam("output-word-graph");
  if (params && params->size()) {
    fileName = params->at(0);

    if (params->size() == 2) {
      outputNBest = Scan<bool>(params->at(1));
    }
  }

  const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();

  outputWordGraphStream << "VERSION=1.0" << endl
                        << "UTTERANCE=" << translationId << endl;

  size_t linkId = 0;
  std::vector < HypothesisStack* >::const_iterator iterStack;
  for (iterStack = ++hypoStackColl.begin() ; iterStack != hypoStackColl.end() ; ++iterStack) {
    const HypothesisStack &stack = **iterStack;
    HypothesisStack::const_iterator iterHypo;
    for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo) {
      const Hypothesis *hypo = *iterHypo;
      OutputWordGraph(outputWordGraphStream, hypo, linkId);

      if (outputNBest) {
        const ArcList *arcList = hypo->GetArcList();
        if (arcList != NULL) {
          ArcList::const_iterator iterArcList;
          for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
            const Hypothesis *loserHypo = *iterArcList;
            OutputWordGraph(outputWordGraphStream, loserHypo, linkId);
          }
        }
      } //if (outputNBest)
    } //for (iterHypo
  } // for (iterStack
}

void Manager::GetSearchGraph(vector<SearchGraphNode>& searchGraph) const
{
  std::map < int, bool > connected;
  std::map < int, int > forward;
  std::map < int, double > forwardScore;

  // *** find connected hypotheses ***
  std::vector< const Hypothesis *> connectedList;
  GetConnectedGraph(&connected, &connectedList);

  // ** compute best forward path for each hypothesis *** //

  // forward cost of hypotheses on final stack is 0
  const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();
  const HypothesisStack &finalStack = *hypoStackColl.back();
  HypothesisStack::const_iterator iterHypo;
  for (iterHypo = finalStack.begin() ; iterHypo != finalStack.end() ; ++iterHypo) {
    const Hypothesis *hypo = *iterHypo;
    forwardScore[ hypo->GetId() ] = 0.0f;
    forward[ hypo->GetId() ] = -1;
  }

  // compete for best forward score of previous hypothesis
  std::vector < HypothesisStack* >::const_iterator iterStack;
  for (iterStack = --hypoStackColl.end() ; iterStack != hypoStackColl.begin() ; --iterStack) {
    const HypothesisStack &stack = **iterStack;
    HypothesisStack::const_iterator iterHypo;
    for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo) {
      const Hypothesis *hypo = *iterHypo;
      if (connected.find( hypo->GetId() ) != connected.end()) {
        // make a play for previous hypothesis
        const Hypothesis *prevHypo = hypo->GetPrevHypo();
        double fscore = forwardScore[ hypo->GetId() ] +
                        hypo->GetScore() - prevHypo->GetScore();
        if (forwardScore.find( prevHypo->GetId() ) == forwardScore.end()
            || forwardScore.find( prevHypo->GetId() )->second < fscore) {
          forwardScore[ prevHypo->GetId() ] = fscore;
          forward[ prevHypo->GetId() ] = hypo->GetId();
        }
        // all arcs also make a play
        const ArcList *arcList = hypo->GetArcList();
        if (arcList != NULL) {
          ArcList::const_iterator iterArcList;
          for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
            const Hypothesis *loserHypo = *iterArcList;
            // make a play
            const Hypothesis *loserPrevHypo = loserHypo->GetPrevHypo();
            double fscore = forwardScore[ hypo->GetId() ] +
                            loserHypo->GetScore() - loserPrevHypo->GetScore();
            if (forwardScore.find( loserPrevHypo->GetId() ) == forwardScore.end()
                || forwardScore.find( loserPrevHypo->GetId() )->second < fscore) {
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
  for (iterStack = hypoStackColl.begin() ; iterStack != hypoStackColl.end() ; ++iterStack) {
    const HypothesisStack &stack = **iterStack;
    HypothesisStack::const_iterator iterHypo;
    for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo) {
      const Hypothesis *hypo = *iterHypo;
      if (connected.find( hypo->GetId() ) != connected.end()) {
        searchGraph.push_back(SearchGraphNode(hypo,NULL,forward[hypo->GetId()],
                                              forwardScore[hypo->GetId()]));

        const ArcList *arcList = hypo->GetArcList();
        if (arcList != NULL) {
          ArcList::const_iterator iterArcList;
          for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
            const Hypothesis *loserHypo = *iterArcList;
            searchGraph.push_back(SearchGraphNode(loserHypo,hypo,
                                                  forward[hypo->GetId()], forwardScore[hypo->GetId()]));
          }
        } // end if arcList empty
      } // end if connected
    } // end for iterHypo
  } // end for iterStack

}

void Manager::OutputFeatureWeightsForSLF(std::ostream &outputSearchGraphStream) const
{
  outputSearchGraphStream.setf(std::ios::fixed);
  outputSearchGraphStream.precision(6);

  const vector<const StatelessFeatureFunction*>& slf  = StatelessFeatureFunction::GetStatelessFeatureFunctions();
  const vector<const StatefulFeatureFunction*>& sff   = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  size_t featureIndex = 1;
  for (size_t i = 0; i < sff.size(); ++i) {
    featureIndex = OutputFeatureWeightsForSLF(featureIndex, sff[i], outputSearchGraphStream);
  }
  for (size_t i = 0; i < slf.size(); ++i) {
    /*
    if (slf[i]->GetScoreProducerWeightShortName() != "u" &&
          slf[i]->GetScoreProducerWeightShortName() != "tm" &&
          slf[i]->GetScoreProducerWeightShortName() != "I" &&
          slf[i]->GetScoreProducerWeightShortName() != "g")
    */
    {
      featureIndex = OutputFeatureWeightsForSLF(featureIndex, slf[i], outputSearchGraphStream);
    }
  }
  const vector<PhraseDictionary*>& pds = PhraseDictionary::GetColl();
  for( size_t i=0; i<pds.size(); i++ ) {
    featureIndex = OutputFeatureWeightsForSLF(featureIndex, pds[i], outputSearchGraphStream);
  }
  const vector<GenerationDictionary*>& gds = GenerationDictionary::GetColl();
  for( size_t i=0; i<gds.size(); i++ ) {
    featureIndex = OutputFeatureWeightsForSLF(featureIndex, gds[i], outputSearchGraphStream);
  }
}

void Manager::OutputFeatureValuesForSLF(const Hypothesis* hypo, bool zeros, std::ostream &outputSearchGraphStream) const
{
  outputSearchGraphStream.setf(std::ios::fixed);
  outputSearchGraphStream.precision(6);

  // outputSearchGraphStream << endl;
  // outputSearchGraphStream << (*hypo) << endl;
  // const ScoreComponentCollection& scoreCollection = hypo->GetScoreBreakdown();
  // outputSearchGraphStream << scoreCollection << endl;

  const vector<const StatelessFeatureFunction*>& slf =StatelessFeatureFunction::GetStatelessFeatureFunctions();
  const vector<const StatefulFeatureFunction*>& sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  size_t featureIndex = 1;
  for (size_t i = 0; i < sff.size(); ++i) {
    featureIndex = OutputFeatureValuesForSLF(featureIndex, zeros, hypo, sff[i], outputSearchGraphStream);
  }
  for (size_t i = 0; i < slf.size(); ++i) {
    /*
    if (slf[i]->GetScoreProducerWeightShortName() != "u" &&
          slf[i]->GetScoreProducerWeightShortName() != "tm" &&
          slf[i]->GetScoreProducerWeightShortName() != "I" &&
          slf[i]->GetScoreProducerWeightShortName() != "g")
    */
    {
      featureIndex = OutputFeatureValuesForSLF(featureIndex, zeros, hypo, slf[i], outputSearchGraphStream);
    }
  }
  const vector<PhraseDictionary*>& pds = PhraseDictionary::GetColl();
  for( size_t i=0; i<pds.size(); i++ ) {
    featureIndex = OutputFeatureValuesForSLF(featureIndex, zeros, hypo, pds[i], outputSearchGraphStream);
  }
  const vector<GenerationDictionary*>& gds = GenerationDictionary::GetColl();
  for( size_t i=0; i<gds.size(); i++ ) {
    featureIndex = OutputFeatureValuesForSLF(featureIndex, zeros, hypo, gds[i], outputSearchGraphStream);
  }

}

void Manager::OutputFeatureValuesForHypergraph(const Hypothesis* hypo, std::ostream &outputSearchGraphStream) const
{
  outputSearchGraphStream.setf(std::ios::fixed);
  outputSearchGraphStream.precision(6);
  ScoreComponentCollection scores = hypo->GetScoreBreakdown();
  const Hypothesis *prevHypo = hypo->GetPrevHypo();
  if (prevHypo) {
    scores.MinusEquals(prevHypo->GetScoreBreakdown());
  }
  scores.Save(outputSearchGraphStream, false);
}


size_t Manager::OutputFeatureWeightsForSLF(size_t index, const FeatureFunction* ff, std::ostream &outputSearchGraphStream) const
{
  size_t numScoreComps = ff->GetNumScoreComponents();
  if (numScoreComps != 0) {
    vector<float> values = StaticData::Instance().GetAllWeights().GetScoresForProducer(ff);
    for (size_t i = 0; i < numScoreComps; ++i) {
      outputSearchGraphStream << "# " << ff->GetScoreProducerDescription()
                              << " "  << ff->GetScoreProducerDescription()
                              << " "  << (i+1) << " of " << numScoreComps << endl
                              << "x"  << (index+i) << "scale=" << values[i] << endl;
    }
    return index+numScoreComps;
  } else {
    cerr << "Sparse features are not supported when outputting HTK standard lattice format" << endl;
    assert(false);
    return 0;
  }
}

size_t Manager::OutputFeatureValuesForSLF(size_t index, bool zeros, const Hypothesis* hypo, const FeatureFunction* ff, std::ostream &outputSearchGraphStream) const
{

  // { const FeatureFunction* sp = ff;
  //   const FVector& m_scores = scoreCollection.GetScoresVector();
  //   FVector& scores = const_cast<FVector&>(m_scores);
  //   std::string prefix = sp->GetScoreProducerDescription() + FName::SEP;
  //   // std::cout << "prefix==" << prefix << endl;
  //   // cout << "m_scores==" << m_scores << endl;
  //   // cout << "m_scores.size()==" << m_scores.size() << endl;
  //   // cout << "m_scores.coreSize()==" << m_scores.coreSize() << endl;
  //   // cout << "m_scores.cbegin() ?= m_scores.cend()\t" <<  (m_scores.cbegin() == m_scores.cend()) << endl;


  //   // for(FVector::FNVmap::const_iterator i = m_scores.cbegin(); i != m_scores.cend(); i++) {
  //   //   std::cout<<prefix << "\t" << (i->first) << "\t" << (i->second) << std::endl;
  //   // }
  //   for(int i=0, n=v.size(); i<n; i+=1) {
  //     //      outputSearchGraphStream << prefix << i << "==" << v[i] << std::endl;

  //   }
  // }

  // FVector featureValues = scoreCollection.GetVectorForProducer(ff);
  // outputSearchGraphStream << featureValues << endl;
  const ScoreComponentCollection& scoreCollection = hypo->GetScoreBreakdown();

  vector<float> featureValues = scoreCollection.GetScoresForProducer(ff);
  size_t numScoreComps = featureValues.size();//featureValues.coreSize();
  //  if (numScoreComps != ScoreProducer::unlimited) {
  // vector<float> values = StaticData::Instance().GetAllWeights().GetScoresForProducer(ff);
  for (size_t i = 0; i < numScoreComps; ++i) {
    outputSearchGraphStream << "x"  << (index+i) << "=" << ((zeros) ? 0.0 : featureValues[i]) << " ";
  }
  return index+numScoreComps;
  // } else {
  //   cerr << "Sparse features are not supported when outputting HTK standard lattice format" << endl;
  //   assert(false);
  //   return 0;
  // }
}

/**! Output search graph in hypergraph format of Kenneth Heafield's lazy hypergraph decoder */
void Manager::OutputSearchGraphAsHypergraph(std::ostream &outputSearchGraphStream) const
{

  VERBOSE(2,"Getting search graph to output as hypergraph for sentence " << m_source.GetTranslationId() << std::endl)

  vector<SearchGraphNode> searchGraph;
  GetSearchGraph(searchGraph);


  map<int,int> mosesIDToHypergraphID;
  // map<int,int> hypergraphIDToMosesID;
  set<int> terminalNodes;
  multimap<int,int> hypergraphIDToArcs;

  VERBOSE(2,"Gathering information about search graph to output as hypergraph for sentence " << m_source.GetTranslationId() << std::endl)

  long numNodes = 0;
  long endNode = 0;
  {
    long hypergraphHypothesisID = 0;
    for (size_t arcNumber = 0, size=searchGraph.size(); arcNumber < size; ++arcNumber) {

      // Get an id number for the previous hypothesis
      const Hypothesis *prevHypo = searchGraph[arcNumber].hypo->GetPrevHypo();
      if (prevHypo!=NULL) {
        int mosesPrevHypothesisID = prevHypo->GetId();
        if (mosesIDToHypergraphID.count(mosesPrevHypothesisID) == 0) {
          mosesIDToHypergraphID[mosesPrevHypothesisID] = hypergraphHypothesisID;
          //	hypergraphIDToMosesID[hypergraphHypothesisID] = mosesPrevHypothesisID;
          hypergraphHypothesisID += 1;
        }
      }

      // Get an id number for this hypothesis
      int mosesHypothesisID;
      if (searchGraph[arcNumber].recombinationHypo) {
        mosesHypothesisID = searchGraph[arcNumber].recombinationHypo->GetId();
      } else {
        mosesHypothesisID = searchGraph[arcNumber].hypo->GetId();
      }

      if (mosesIDToHypergraphID.count(mosesHypothesisID) == 0) {

        mosesIDToHypergraphID[mosesHypothesisID] = hypergraphHypothesisID;
        //      hypergraphIDToMosesID[hypergraphHypothesisID] = mosesHypothesisID;

        bool terminalNode = (searchGraph[arcNumber].forward == -1);
        if (terminalNode) {
          // Final arc to end node, representing the end of the sentence </s>
          terminalNodes.insert(hypergraphHypothesisID);
        }

        hypergraphHypothesisID += 1;
      }

      // Record that this arc ends at this node
      hypergraphIDToArcs.insert(pair<int,int>(mosesIDToHypergraphID[mosesHypothesisID],arcNumber));

    }

    // Unique end node
    endNode = hypergraphHypothesisID;
    //    mosesIDToHypergraphID[hypergraphHypothesisID] = hypergraphHypothesisID;
    numNodes = endNode + 1;

  }


  long numArcs = searchGraph.size() + terminalNodes.size();

  //Header
  outputSearchGraphStream << "# target ||| features ||| source-covered" << endl;

  // Print number of nodes and arcs
  outputSearchGraphStream << numNodes << " " << numArcs << endl;

  VERBOSE(2,"Search graph to output as hypergraph for sentence " << m_source.GetTranslationId()
          << " contains " << numArcs << " arcs and " << numNodes << " nodes" << std::endl)

  VERBOSE(2,"Outputting search graph to output as hypergraph for sentence " << m_source.GetTranslationId() << std::endl)


  for (int hypergraphHypothesisID=0; hypergraphHypothesisID < endNode; hypergraphHypothesisID+=1) {
    if (hypergraphHypothesisID % 100000 == 0) {
      VERBOSE(2,"Processed " << hypergraphHypothesisID << " of " << numNodes << " hypergraph nodes for sentence " << m_source.GetTranslationId() << std::endl);
    }
    //    int mosesID = hypergraphIDToMosesID[hypergraphHypothesisID];
    size_t count = hypergraphIDToArcs.count(hypergraphHypothesisID);
    //    VERBOSE(2,"Hypergraph node " << hypergraphHypothesisID << " has " << count << " incoming arcs" << std::endl)
    if (count > 0) {
      outputSearchGraphStream << "# node " << hypergraphHypothesisID << endl;
      outputSearchGraphStream << count << "\n";

      pair<multimap<int,int>::iterator, multimap<int,int>::iterator> range =
        hypergraphIDToArcs.equal_range(hypergraphHypothesisID);
      for (multimap<int,int>::iterator it=range.first; it!=range.second; ++it) {
        int lineNumber = (*it).second;
        const Hypothesis *thisHypo = searchGraph[lineNumber].hypo;
        int mosesHypothesisID;// = thisHypo->GetId();
        if (searchGraph[lineNumber].recombinationHypo) {
          mosesHypothesisID = searchGraph[lineNumber].recombinationHypo->GetId();
        } else {
          mosesHypothesisID = searchGraph[lineNumber].hypo->GetId();
        }
        //	int actualHypergraphHypothesisID = mosesIDToHypergraphID[mosesHypothesisID];
        UTIL_THROW_IF2(
          (hypergraphHypothesisID != mosesIDToHypergraphID[mosesHypothesisID]),
          "Error while writing search lattice as hypergraph for sentence " << m_source.GetTranslationId() << ". " <<
          "Moses node " << mosesHypothesisID << " was expected to have hypergraph id " << hypergraphHypothesisID <<
          ", but actually had hypergraph id " << mosesIDToHypergraphID[mosesHypothesisID] <<
          ". There are " << numNodes << " nodes in the search lattice."
        );

        const Hypothesis *prevHypo = thisHypo->GetPrevHypo();
        if (prevHypo==NULL) {
          //	VERBOSE(2,"Hypergraph node " << hypergraphHypothesisID << " start of sentence" << std::endl)
          outputSearchGraphStream << "<s> |||  ||| 0\n";
        } else {
          int startNode = mosesIDToHypergraphID[prevHypo->GetId()];
          //	  VERBOSE(2,"Hypergraph node " << hypergraphHypothesisID << " has parent node " << startNode << std::endl)
          UTIL_THROW_IF2(
            (startNode >= hypergraphHypothesisID),
            "Error while writing search lattice as hypergraph for sentence" << m_source.GetTranslationId() << ". " <<
            "The nodes must be output in topological order. The code attempted to violate this restriction."
          );

          const TargetPhrase &targetPhrase = thisHypo->GetCurrTargetPhrase();
          int targetWordCount = targetPhrase.GetSize();

          outputSearchGraphStream << "[" << startNode << "] ";
          for (int targetWordIndex=0; targetWordIndex<targetWordCount; targetWordIndex+=1) {
            outputSearchGraphStream << targetPhrase.GetWord(targetWordIndex)[0]->GetString() << " ";
          }
          outputSearchGraphStream << " ||| ";
          OutputFeatureValuesForHypergraph(thisHypo, outputSearchGraphStream);
          outputSearchGraphStream << " ||| " << thisHypo->GetWordsBitmap().GetNumWordsCovered();
          outputSearchGraphStream << "\n";
        }
      }
    }
  }

  // Print node and arc(s) for end of sentence </s>
  outputSearchGraphStream << "# node " << endNode << endl;
  outputSearchGraphStream << terminalNodes.size() << "\n";
  for (set<int>::iterator it=terminalNodes.begin(); it!=terminalNodes.end(); ++it) {
    outputSearchGraphStream << "[" << (*it) << "] </s> |||  ||| " << GetSource().GetSize() << "\n";
  }

}


/**! Output search graph in HTK standard lattice format (SLF) */
void Manager::OutputSearchGraphAsSLF(long translationId, std::ostream &outputSearchGraphStream) const
{

  vector<SearchGraphNode> searchGraph;
  GetSearchGraph(searchGraph);

  long numArcs = 0;
  long numNodes = 0;

  map<int,int> nodes;
  set<int> terminalNodes;

  // Unique start node
  nodes[0] = 0;

  for (size_t arcNumber = 0; arcNumber < searchGraph.size(); ++arcNumber) {

    int targetWordCount = searchGraph[arcNumber].hypo->GetCurrTargetPhrase().GetSize();
    numArcs += targetWordCount;

    int hypothesisID = searchGraph[arcNumber].hypo->GetId();
    if (nodes.count(hypothesisID) == 0) {

      numNodes += targetWordCount;
      nodes[hypothesisID] = numNodes;
      //numNodes += 1;

      bool terminalNode = (searchGraph[arcNumber].forward == -1);
      if (terminalNode) {
        numArcs += 1;
      }
    }

  }
  numNodes += 1;

  // Unique end node
  nodes[numNodes] = numNodes;

  outputSearchGraphStream << "UTTERANCE=Sentence_" << translationId << endl;
  outputSearchGraphStream << "VERSION=1.1" << endl;
  outputSearchGraphStream << "base=2.71828182845905" << endl;
  outputSearchGraphStream << "NODES=" << (numNodes+1) << endl;
  outputSearchGraphStream << "LINKS=" << numArcs  << endl;

  OutputFeatureWeightsForSLF(outputSearchGraphStream);

  for (size_t arcNumber = 0, lineNumber = 0; lineNumber < searchGraph.size(); ++lineNumber) {
    const Hypothesis *thisHypo = searchGraph[lineNumber].hypo;
    const Hypothesis *prevHypo = thisHypo->GetPrevHypo();
    if (prevHypo) {

      int startNode = nodes[prevHypo->GetId()];
      int endNode   = nodes[thisHypo->GetId()];
      bool terminalNode = (searchGraph[lineNumber].forward == -1);
      const TargetPhrase &targetPhrase = thisHypo->GetCurrTargetPhrase();
      int targetWordCount = targetPhrase.GetSize();

      for (int targetWordIndex=0; targetWordIndex<targetWordCount; targetWordIndex+=1) {
        int x = (targetWordCount-targetWordIndex);

        outputSearchGraphStream <<  "J=" << arcNumber;

        if (targetWordIndex==0) {
          outputSearchGraphStream << " S=" << startNode;
        } else {
          outputSearchGraphStream << " S=" << endNode - x;
        }

        outputSearchGraphStream << " E=" << endNode - (x-1)
                                << " W=" << targetPhrase.GetWord(targetWordIndex);

        OutputFeatureValuesForSLF(thisHypo, (targetWordIndex>0), outputSearchGraphStream);

        outputSearchGraphStream  << endl;

        arcNumber += 1;
      }

      if (terminalNode && terminalNodes.count(endNode) == 0) {
        terminalNodes.insert(endNode);
        outputSearchGraphStream <<  "J="   << arcNumber
                                << " S="   << endNode
                                << " E="   << numNodes
                                << endl;
        arcNumber += 1;
      }
    }
  }

}

void OutputSearchNode(long translationId, std::ostream &outputSearchGraphStream,
                      const SearchGraphNode& searchNode)
{
  const vector<FactorType> &outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
  bool extendedFormat = StaticData::Instance().GetOutputSearchGraphExtended();
  outputSearchGraphStream << translationId;

  // special case: initial hypothesis
  if ( searchNode.hypo->GetId() == 0 ) {
    outputSearchGraphStream << " hyp=0 stack=0";
    if (extendedFormat) {
      outputSearchGraphStream << " forward=" << searchNode.forward	<< " fscore=" << searchNode.fscore;
    }
    outputSearchGraphStream << endl;
    return;
  }

  const Hypothesis *prevHypo = searchNode.hypo->GetPrevHypo();

  // output in traditional format
  if (!extendedFormat) {
    outputSearchGraphStream << " hyp=" << searchNode.hypo->GetId()
                            << " stack=" << searchNode.hypo->GetWordsBitmap().GetNumWordsCovered()
                            << " back=" << prevHypo->GetId()
                            << " score=" << searchNode.hypo->GetScore()
                            << " transition=" << (searchNode.hypo->GetScore() - prevHypo->GetScore());

    if (searchNode.recombinationHypo != NULL)
      outputSearchGraphStream << " recombined=" << searchNode.recombinationHypo->GetId();

    outputSearchGraphStream << " forward=" << searchNode.forward	<< " fscore=" << searchNode.fscore
                            << " covered=" << searchNode.hypo->GetCurrSourceWordsRange().GetStartPos()
                            << "-" << searchNode.hypo->GetCurrSourceWordsRange().GetEndPos()
                            << " out=" << searchNode.hypo->GetCurrTargetPhrase().GetStringRep(outputFactorOrder)
                            << endl;
    return;
  }

  // output in extended format
//  if (searchNode.recombinationHypo != NULL)
//    outputSearchGraphStream << " hyp=" << searchNode.recombinationHypo->GetId();
//  else
  outputSearchGraphStream << " hyp=" << searchNode.hypo->GetId();

  outputSearchGraphStream << " stack=" << searchNode.hypo->GetWordsBitmap().GetNumWordsCovered()
                          << " back=" << prevHypo->GetId()
                          << " score=" << searchNode.hypo->GetScore()
                          << " transition=" << (searchNode.hypo->GetScore() - prevHypo->GetScore());

  if (searchNode.recombinationHypo != NULL)
    outputSearchGraphStream << " recombined=" << searchNode.recombinationHypo->GetId();

  outputSearchGraphStream << " forward=" << searchNode.forward	<< " fscore=" << searchNode.fscore
                          << " covered=" << searchNode.hypo->GetCurrSourceWordsRange().GetStartPos()
                          << "-" << searchNode.hypo->GetCurrSourceWordsRange().GetEndPos();

  // Modified so that -osgx is a superset of -osg (GST Oct 2011)
  ScoreComponentCollection scoreBreakdown = searchNode.hypo->GetScoreBreakdown();
  scoreBreakdown.MinusEquals( prevHypo->GetScoreBreakdown() );
  //outputSearchGraphStream << " scores = [ " << StaticData::Instance().GetAllWeights();
  outputSearchGraphStream << " scores=\"" << scoreBreakdown << "\"";

  outputSearchGraphStream << " out=\"" << searchNode.hypo->GetSourcePhraseStringRep() << "|" <<
                          searchNode.hypo->GetCurrTargetPhrase().GetStringRep(outputFactorOrder) << "\"" << endl;
//  outputSearchGraphStream << " out=" << searchNode.hypo->GetCurrTargetPhrase().GetStringRep(outputFactorOrder) << endl;
}

void Manager::GetConnectedGraph(
  std::map< int, bool >* pConnected,
  std::vector< const Hypothesis* >* pConnectedList) const
{
  std::map < int, bool >& connected = *pConnected;
  std::vector< const Hypothesis *>& connectedList = *pConnectedList;

  // start with the ones in the final stack
  const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();
  const HypothesisStack &finalStack = *hypoStackColl.back();
  HypothesisStack::const_iterator iterHypo;
  for (iterHypo = finalStack.begin() ; iterHypo != finalStack.end() ; ++iterHypo) {
    const Hypothesis *hypo = *iterHypo;
    connected[ hypo->GetId() ] = true;
    connectedList.push_back( hypo );
  }
  // move back from known connected hypotheses
  for(size_t i=0; i<connectedList.size(); i++) {
    const Hypothesis *hypo = connectedList[i];

    // add back pointer
    const Hypothesis *prevHypo = hypo->GetPrevHypo();
    if (prevHypo && prevHypo->GetId() > 0 // don't add empty hypothesis
        && connected.find( prevHypo->GetId() ) == connected.end()) { // don't add already added
      connected[ prevHypo->GetId() ] = true;
      connectedList.push_back( prevHypo );
    }

    // add arcs
    const ArcList *arcList = hypo->GetArcList();
    if (arcList != NULL) {
      ArcList::const_iterator iterArcList;
      for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
        const Hypothesis *loserHypo = *iterArcList;
        if (connected.find( loserHypo->GetId() ) == connected.end()) { // don't add already added
          connected[ loserHypo->GetId() ] = true;
          connectedList.push_back( loserHypo );
        }
      }
    }
  }
}

void Manager::GetWinnerConnectedGraph(
  std::map< int, bool >* pConnected,
  std::vector< const Hypothesis* >* pConnectedList) const
{
  std::map < int, bool >& connected = *pConnected;
  std::vector< const Hypothesis *>& connectedList = *pConnectedList;

  // start with the ones in the final stack
  const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();
  const HypothesisStack &finalStack = *hypoStackColl.back();
  HypothesisStack::const_iterator iterHypo;
  for (iterHypo = finalStack.begin() ; iterHypo != finalStack.end() ; ++iterHypo) {
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
        && connected.find( prevHypo->GetId() ) == connected.end()) { // don't add already added
      connected[ prevHypo->GetId() ] = true;
      connectedList.push_back( prevHypo );
    }

    // add arcs
    const ArcList *arcList = hypo->GetArcList();
    if (arcList != NULL) {
      ArcList::const_iterator iterArcList;
      for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
        const Hypothesis *loserHypo = *iterArcList;
        if (connected.find( loserHypo->GetPrevHypo()->GetId() ) == connected.end() && loserHypo->GetPrevHypo()->GetId() > 0) { // don't add already added & don't add hyp 0
          connected[ loserHypo->GetPrevHypo()->GetId() ] = true;
          connectedList.push_back( loserHypo->GetPrevHypo() );
        }
      }
    }
  }
}


#ifdef HAVE_PROTOBUF

void SerializeEdgeInfo(const Hypothesis* hypo, hgmert::Hypergraph_Edge* edge)
{
  hgmert::Rule* rule = edge->mutable_rule();
  hypo->GetCurrTargetPhrase().WriteToRulePB(rule);
  const Hypothesis* prev = hypo->GetPrevHypo();
  // if the feature values are empty, they default to 0
  if (!prev) return;
  // score breakdown is an aggregate (forward) quantity, but the exported
  // graph object just wants the feature values on the edges
  const ScoreComponentCollection& scores = hypo->GetScoreBreakdown();
  const ScoreComponentCollection& pscores = prev->GetScoreBreakdown();
  for (unsigned int i = 0; i < scores.size(); ++i)
    edge->add_feature_values((scores[i] - pscores[i]) * -1.0);
}

hgmert::Hypergraph_Node* GetHGNode(
  const Hypothesis* hypo,
  std::map< int, int>* i2hgnode,
  hgmert::Hypergraph* hg,
  int* hgNodeIdx)
{
  hgmert::Hypergraph_Node* hgnode;
  std::map < int, int >::iterator idxi = i2hgnode->find(hypo->GetId());
  if (idxi == i2hgnode->end()) {
    *hgNodeIdx = ((*i2hgnode)[hypo->GetId()] = hg->nodes_size());
    hgnode = hg->add_nodes();
  } else {
    *hgNodeIdx = idxi->second;
    hgnode = hg->mutable_nodes(*hgNodeIdx);
  }
  return hgnode;
}

void Manager::SerializeSearchGraphPB(
  long translationId,
  std::ostream& outputStream) const
{
  using namespace hgmert;
  std::map < int, bool > connected;
  std::map < int, int > i2hgnode;
  std::vector< const Hypothesis *> connectedList;
  GetConnectedGraph(&connected, &connectedList);
  connected[ 0 ] = true;
  Hypergraph hg;
  hg.set_is_sorted(false);
  int num_feats = (*m_search->GetHypothesisStacks().back()->begin())->GetScoreBreakdown().size();
  hg.set_num_features(num_feats);
  StaticData::Instance().GetScoreIndexManager().SerializeFeatureNamesToPB(&hg);
  Hypergraph_Node* goal = hg.add_nodes();  // idx=0 goal node must have idx 0
  Hypergraph_Node* source = hg.add_nodes();  // idx=1
  i2hgnode[-1] = 1; // source node
  const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();
  const HypothesisStack &finalStack = *hypoStackColl.back();
  for (std::vector < HypothesisStack* >::const_iterator iterStack = hypoStackColl.begin();
       iterStack != hypoStackColl.end() ; ++iterStack) {
    const HypothesisStack &stack = **iterStack;
    HypothesisStack::const_iterator iterHypo;

    for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo) {
      const Hypothesis *hypo = *iterHypo;
      bool is_goal = hypo->GetWordsBitmap().IsComplete();
      if (connected.find( hypo->GetId() ) != connected.end()) {
        int headNodeIdx;
        Hypergraph_Node* headNode = GetHGNode(hypo, &i2hgnode, &hg, &headNodeIdx);
        if (is_goal) {
          Hypergraph_Edge* ge = hg.add_edges();
          ge->set_head_node(0);  // goal
          ge->add_tail_nodes(headNodeIdx);
          ge->mutable_rule()->add_trg_words("[X,1]");
        }
        Hypergraph_Edge* edge = hg.add_edges();
        SerializeEdgeInfo(hypo, edge);
        edge->set_head_node(headNodeIdx);
        const Hypothesis* prev = hypo->GetPrevHypo();
        int tailNodeIdx = 1; // source
        if (prev)
          tailNodeIdx = i2hgnode.find(prev->GetId())->second;
        edge->add_tail_nodes(tailNodeIdx);

        const ArcList *arcList = hypo->GetArcList();
        if (arcList != NULL) {
          ArcList::const_iterator iterArcList;
          for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
            const Hypothesis *loserHypo = *iterArcList;
            UTIL_THROW_IF2(!connected[loserHypo->GetId()],
                           "Hypothesis " << loserHypo->GetId() << " is not connected");
            Hypergraph_Edge* edge = hg.add_edges();
            SerializeEdgeInfo(loserHypo, edge);
            edge->set_head_node(headNodeIdx);
            tailNodeIdx = i2hgnode.find(loserHypo->GetPrevHypo()->GetId())->second;
            edge->add_tail_nodes(tailNodeIdx);
          }
        } // end if arcList empty
      } // end if connected
    } // end for iterHypo
  } // end for iterStack
  hg.SerializeToOstream(&outputStream);
}
#endif

void Manager::OutputSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const
{
  vector<SearchGraphNode> searchGraph;
  GetSearchGraph(searchGraph);
  for (size_t i = 0; i < searchGraph.size(); ++i) {
    OutputSearchNode(translationId,outputSearchGraphStream,searchGraph[i]);
  }
}

void Manager::GetForwardBackwardSearchGraph(std::map< int, bool >* pConnected,
    std::vector< const Hypothesis* >* pConnectedList, std::map < const Hypothesis*, set< const Hypothesis* > >* pOutgoingHyps, vector< float>* pFwdBwdScores) const
{
  std::map < int, bool > &connected = *pConnected;
  std::vector< const Hypothesis *>& connectedList = *pConnectedList;
  std::map < int, int > forward;
  std::map < int, double > forwardScore;

  std::map < const Hypothesis*, set <const Hypothesis*> > & outgoingHyps = *pOutgoingHyps;
  vector< float> & estimatedScores = *pFwdBwdScores;

  // *** find connected hypotheses ***
  GetWinnerConnectedGraph(&connected, &connectedList);

  // ** compute best forward path for each hypothesis *** //

  // forward cost of hypotheses on final stack is 0
  const std::vector < HypothesisStack* > &hypoStackColl = m_search->GetHypothesisStacks();
  const HypothesisStack &finalStack = *hypoStackColl.back();
  HypothesisStack::const_iterator iterHypo;
  for (iterHypo = finalStack.begin() ; iterHypo != finalStack.end() ; ++iterHypo) {
    const Hypothesis *hypo = *iterHypo;
    forwardScore[ hypo->GetId() ] = 0.0f;
    forward[ hypo->GetId() ] = -1;
  }

  // compete for best forward score of previous hypothesis
  std::vector < HypothesisStack* >::const_iterator iterStack;
  for (iterStack = --hypoStackColl.end() ; iterStack != hypoStackColl.begin() ; --iterStack) {
    const HypothesisStack &stack = **iterStack;
    HypothesisStack::const_iterator iterHypo;
    for (iterHypo = stack.begin() ; iterHypo != stack.end() ; ++iterHypo) {
      const Hypothesis *hypo = *iterHypo;
      if (connected.find( hypo->GetId() ) != connected.end()) {
        // make a play for previous hypothesis
        const Hypothesis *prevHypo = hypo->GetPrevHypo();
        double fscore = forwardScore[ hypo->GetId() ] +
                        hypo->GetScore() - prevHypo->GetScore();
        if (forwardScore.find( prevHypo->GetId() ) == forwardScore.end()
            || forwardScore.find( prevHypo->GetId() )->second < fscore) {
          forwardScore[ prevHypo->GetId() ] = fscore;
          forward[ prevHypo->GetId() ] = hypo->GetId();
        }
        //store outgoing info
        outgoingHyps[prevHypo].insert(hypo);

        // all arcs also make a play
        const ArcList *arcList = hypo->GetArcList();
        if (arcList != NULL) {
          ArcList::const_iterator iterArcList;
          for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
            const Hypothesis *loserHypo = *iterArcList;
            // make a play
            const Hypothesis *loserPrevHypo = loserHypo->GetPrevHypo();
            double fscore = forwardScore[ hypo->GetId() ] +
                            loserHypo->GetScore() - loserPrevHypo->GetScore();
            if (forwardScore.find( loserPrevHypo->GetId() ) == forwardScore.end()
                || forwardScore.find( loserPrevHypo->GetId() )->second < fscore) {
              forwardScore[ loserPrevHypo->GetId() ] = fscore;
              forward[ loserPrevHypo->GetId() ] = loserHypo->GetId();
            }
            //store outgoing info
            outgoingHyps[loserPrevHypo].insert(hypo);


          } // end for arc list
        } // end if arc list empty
      } // end if hypo connected
    } // end for hypo
  } // end for stack

  for (std::vector< const Hypothesis *>::iterator it = connectedList.begin(); it != connectedList.end(); ++it) {
    float estimatedScore = (*it)->GetScore() + forwardScore[(*it)->GetId()];
    estimatedScores.push_back(estimatedScore);
  }
}


const Hypothesis *Manager::GetBestHypothesis() const
{
  return m_search->GetBestHypothesis();
}

int Manager::GetNextHypoId()
{
  return m_hypoId++;
}

void Manager::ResetSentenceStats(const InputType& source)
{
  m_sentenceStats = std::auto_ptr<SentenceStats>(new SentenceStats(source));
}
SentenceStats& Manager::GetSentenceStats() const
{
  return *m_sentenceStats;

}

void Manager::OutputBest(OutputCollector *collector)  const
{
  const StaticData &staticData = StaticData::Instance();
  long translationId = m_source.GetTranslationId();

  Timer additionalReportingTime;

  // apply decision rule and output best translation(s)
  if (collector) {
    ostringstream out;
    ostringstream debug;
    FixPrecision(debug,PRECISION);

    // all derivations - send them to debug stream
    if (staticData.PrintAllDerivations()) {
      additionalReportingTime.start();
      PrintAllDerivations(translationId, debug);
      additionalReportingTime.stop();
    }

    Timer decisionRuleTime;
    decisionRuleTime.start();

    // MAP decoding: best hypothesis
    const Hypothesis* bestHypo = NULL;
    if (!staticData.UseMBR()) {
      bestHypo = GetBestHypothesis();
      if (bestHypo) {
        if (StaticData::Instance().GetOutputHypoScore()) {
          out << bestHypo->GetTotalScore() << ' ';
        }
        if (staticData.IsPathRecoveryEnabled()) {
          bestHypo->OutputInput(out);
          out << "||| ";
        }

        const PARAM_VEC *params = staticData.GetParameter().GetParam("print-id");
        if (params && params->size() && Scan<bool>(params->at(0)) ) {
          out << translationId << " ";
        }

        if (staticData.GetReportSegmentation() == 2) {
          GetOutputLanguageModelOrder(out, bestHypo);
        }
        bestHypo->OutputBestSurface(
          out,
          staticData.GetOutputFactorOrder(),
          staticData.GetReportSegmentation(),
          staticData.GetReportAllFactors());
        if (staticData.PrintAlignmentInfo()) {
          out << "||| ";
          bestHypo->OutputAlignment(out);
        }

        IFVERBOSE(1) {
          debug << "BEST TRANSLATION: " << *bestHypo << endl;
        }
      } else {
        VERBOSE(1, "NO BEST TRANSLATION" << endl);
      }

      out << endl;
    } // if (!staticData.UseMBR())

    // MBR decoding (n-best MBR, lattice MBR, consensus)
    else {
      // we first need the n-best translations
      size_t nBestSize = staticData.GetMBRSize();
      if (nBestSize <= 0) {
        cerr << "ERROR: negative size for number of MBR candidate translations not allowed (option mbr-size)" << endl;
        exit(1);
      }
      TrellisPathList nBestList;
      CalcNBest(nBestSize, nBestList,true);
      VERBOSE(2,"size of n-best: " << nBestList.GetSize() << " (" << nBestSize << ")" << endl);
      IFVERBOSE(2) {
        PrintUserTime("calculated n-best list for (L)MBR decoding");
      }

      // lattice MBR
      if (staticData.UseLatticeMBR()) {
        if (staticData.IsNBestEnabled()) {
          //lattice mbr nbest
          vector<LatticeMBRSolution> solutions;
          size_t n  = min(nBestSize, staticData.GetNBestSize());
          getLatticeMBRNBest(*this,nBestList,solutions,n);
          OutputLatticeMBRNBest(m_latticeNBestOut, solutions, translationId);
        } else {
          //Lattice MBR decoding
          vector<Word> mbrBestHypo = doLatticeMBR(*this,nBestList);
          OutputBestHypo(mbrBestHypo, translationId, staticData.GetReportSegmentation(),
                         staticData.GetReportAllFactors(),out);
          IFVERBOSE(2) {
            PrintUserTime("finished Lattice MBR decoding");
          }
        }
      }

      // consensus decoding
      else if (staticData.UseConsensusDecoding()) {
        const TrellisPath &conBestHypo = doConsensusDecoding(*this,nBestList);
        OutputBestHypo(conBestHypo, translationId,
                       staticData.GetReportSegmentation(),
                       staticData.GetReportAllFactors(),out);
        OutputAlignment(m_alignmentOut, conBestHypo);
        IFVERBOSE(2) {
          PrintUserTime("finished Consensus decoding");
        }
      }

      // n-best MBR decoding
      else {
        const TrellisPath &mbrBestHypo = doMBR(nBestList);
        OutputBestHypo(mbrBestHypo, translationId,
                       staticData.GetReportSegmentation(),
                       staticData.GetReportAllFactors(),out);
        OutputAlignment(m_alignmentOut, mbrBestHypo);
        IFVERBOSE(2) {
          PrintUserTime("finished MBR decoding");
        }
      }
    }

    // report best translation to output collector
    collector->Write(translationId,out.str(),debug.str());

    decisionRuleTime.stop();
    VERBOSE(1, "Line " << translationId << ": Decision rule took " << decisionRuleTime << " seconds total" << endl);
  } // if (m_ioWrapper.GetSingleBestOutputCollector())

}

void Manager::OutputNBest(OutputCollector *collector) const
{
  if (collector == NULL) {
    return;
  }

  const StaticData &staticData = StaticData::Instance();
  long translationId = m_source.GetTranslationId();

  if (staticData.UseLatticeMBR()) {
    if (staticData.IsNBestEnabled()) {
      collector->Write(translationId, m_latticeNBestOut.str());
    }
  } else {
    TrellisPathList nBestList;
    ostringstream out;
    CalcNBest(staticData.GetNBestSize(), nBestList,staticData.GetDistinctNBest());
    OutputNBest(out, nBestList, staticData.GetOutputFactorOrder(), m_source.GetTranslationId(),
                staticData.GetReportSegmentation());
    collector->Write(m_source.GetTranslationId(), out.str());
  }

}

void Manager::OutputNBest(std::ostream& out
                          , const Moses::TrellisPathList &nBestList
                          , const std::vector<Moses::FactorType>& outputFactorOrder
                          , long translationId
                          , char reportSegmentation) const
{
  const StaticData &staticData = StaticData::Instance();
  bool reportAllFactors = staticData.GetReportAllFactorsNBest();
  bool includeSegmentation = staticData.NBestIncludesSegmentation();
  bool includeWordAlignment = staticData.PrintAlignmentInfoInNbest();

  TrellisPathList::const_iterator iter;
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
    const TrellisPath &path = **iter;
    const std::vector<const Hypothesis *> &edges = path.GetEdges();

    // print the surface factor of the translation
    out << translationId << " ||| ";
    for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
      const Hypothesis &edge = *edges[currEdge];
      OutputSurface(out, edge, outputFactorOrder, reportSegmentation, reportAllFactors);
    }
    out << " |||";

    // print scores with feature names
    path.GetScoreBreakdown().OutputAllFeatureScores(out );

    // total
    out << " ||| " << path.GetTotalScore();

    //phrase-to-phrase segmentation
    if (includeSegmentation) {
      out << " |||";
      for (int currEdge = (int)edges.size() - 2 ; currEdge >= 0 ; currEdge--) {
        const Hypothesis &edge = *edges[currEdge];
        const WordsRange &sourceRange = edge.GetCurrSourceWordsRange();
        WordsRange targetRange = path.GetTargetWordsRange(edge);
        out << " " << sourceRange.GetStartPos();
        if (sourceRange.GetStartPos() < sourceRange.GetEndPos()) {
          out << "-" << sourceRange.GetEndPos();
        }
        out<< "=" << targetRange.GetStartPos();
        if (targetRange.GetStartPos() < targetRange.GetEndPos()) {
          out<< "-" << targetRange.GetEndPos();
        }
      }
    }

    if (includeWordAlignment) {
      out << " ||| ";
      for (int currEdge = (int)edges.size() - 2 ; currEdge >= 0 ; currEdge--) {
        const Hypothesis &edge = *edges[currEdge];
        const WordsRange &sourceRange = edge.GetCurrSourceWordsRange();
        WordsRange targetRange = path.GetTargetWordsRange(edge);
        const int sourceOffset = sourceRange.GetStartPos();
        const int targetOffset = targetRange.GetStartPos();
        const AlignmentInfo &ai = edge.GetCurrTargetPhrase().GetAlignTerm();

        OutputAlignment(out, ai, sourceOffset, targetOffset);

      }
    }

    if (StaticData::Instance().IsPathRecoveryEnabled()) {
      out << " ||| ";
      OutputInput(out, edges[0]);
    }

    out << endl;
  }

  out << std::flush;
}

//////////////////////////////////////////////////////////////////////////
/***
 * print surface factor only for the given phrase
 */
void Manager::OutputSurface(std::ostream &out, const Hypothesis &edge, const std::vector<FactorType> &outputFactorOrder,
                            char reportSegmentation, bool reportAllFactors) const
{
  UTIL_THROW_IF2(outputFactorOrder.size() == 0,
                 "Must specific at least 1 output factor");
  const TargetPhrase& phrase = edge.GetCurrTargetPhrase();
  bool markUnknown = StaticData::Instance().GetMarkUnknown();
  if (reportAllFactors == true) {
    out << phrase;
  } else {
    FactorType placeholderFactor = StaticData::Instance().GetPlaceholderFactor();

    std::map<size_t, const Factor*> placeholders;
    if (placeholderFactor != NOT_FOUND) {
      // creates map of target position -> factor for placeholders
      placeholders = GetPlaceholders(edge, placeholderFactor);
    }

    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);

      if (placeholders.size()) {
        // do placeholders
        std::map<size_t, const Factor*>::const_iterator iter = placeholders.find(pos);
        if (iter != placeholders.end()) {
          factor = iter->second;
        }
      }

      UTIL_THROW_IF2(factor == NULL,
                     "No factor 0 at position " << pos);

      //preface surface form with UNK if marking unknowns
      const Word &word = phrase.GetWord(pos);
      if(markUnknown && word.IsOOV()) {
        out << "UNK" << *factor;
      } else {
        out << *factor;
      }

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
        UTIL_THROW_IF2(factor == NULL,
                       "No factor " << i << " at position " << pos);

        out << "|" << *factor;
      }
      out << " ";
    }
  }

  // trace ("report segmentation") option "-t" / "-tt"
  if (reportSegmentation > 0 && phrase.GetSize() > 0) {
    const WordsRange &sourceRange = edge.GetCurrSourceWordsRange();
    const int sourceStart = sourceRange.GetStartPos();
    const int sourceEnd = sourceRange.GetEndPos();
    out << "|" << sourceStart << "-" << sourceEnd;    // enriched "-tt"
    if (reportSegmentation == 2) {
      out << ",wa=";
      const AlignmentInfo &ai = edge.GetCurrTargetPhrase().GetAlignTerm();
      OutputAlignment(out, ai, 0, 0);
      out << ",total=";
      out << edge.GetScore() - edge.GetPrevHypo()->GetScore();
      out << ",";
      ScoreComponentCollection scoreBreakdown(edge.GetScoreBreakdown());
      scoreBreakdown.MinusEquals(edge.GetPrevHypo()->GetScoreBreakdown());
      scoreBreakdown.OutputAllFeatureScores(out);
    }
    out << "| ";
  }
}

void Manager::OutputAlignment(ostream &out, const AlignmentInfo &ai, size_t sourceOffset, size_t targetOffset) const
{
  typedef std::vector< const std::pair<size_t,size_t>* > AlignVec;
  AlignVec alignments = ai.GetSortedAlignments();

  AlignVec::const_iterator it;
  for (it = alignments.begin(); it != alignments.end(); ++it) {
    const std::pair<size_t,size_t> &alignment = **it;
    out << alignment.first + sourceOffset << "-" << alignment.second + targetOffset << " ";
  }

}

void Manager::OutputInput(std::ostream& os, const Hypothesis* hypo) const
{
  size_t len = hypo->GetInput().GetSize();
  std::vector<const Phrase*> inp_phrases(len, 0);
  OutputInput(inp_phrases, hypo);
  for (size_t i=0; i<len; ++i)
    if (inp_phrases[i]) os << *inp_phrases[i];
}

void Manager::OutputInput(std::vector<const Phrase*>& map, const Hypothesis* hypo) const
{
  if (hypo->GetPrevHypo()) {
    OutputInput(map, hypo->GetPrevHypo());
    map[hypo->GetCurrSourceWordsRange().GetStartPos()] = &hypo->GetTranslationOption().GetInputPath().GetPhrase();
  }
}

std::map<size_t, const Factor*> Manager::GetPlaceholders(const Hypothesis &hypo, FactorType placeholderFactor) const
{
  const InputPath &inputPath = hypo.GetTranslationOption().GetInputPath();
  const Phrase &inputPhrase = inputPath.GetPhrase();

  std::map<size_t, const Factor*> ret;

  for (size_t sourcePos = 0; sourcePos < inputPhrase.GetSize(); ++sourcePos) {
    const Factor *factor = inputPhrase.GetFactor(sourcePos, placeholderFactor);
    if (factor) {
      std::set<size_t> targetPos = hypo.GetTranslationOption().GetTargetPhrase().GetAlignTerm().GetAlignmentsForSource(sourcePos);
      UTIL_THROW_IF2(targetPos.size() != 1,
                     "Placeholder should be aligned to 1, and only 1, word");
      ret[*targetPos.begin()] = factor;
    }
  }

  return ret;
}

void Manager::OutputLatticeSamples(OutputCollector *collector) const
{
  const StaticData &staticData = StaticData::Instance();
  if (collector) {
    TrellisPathList latticeSamples;
    ostringstream out;
    CalcLatticeSamples(staticData.GetLatticeSamplesSize(), latticeSamples);
    OutputNBest(out,latticeSamples, staticData.GetOutputFactorOrder(), m_source.GetTranslationId(),
                staticData.GetReportSegmentation());
    collector->Write(m_source.GetTranslationId(), out.str());
  }

}

void Manager::OutputAlignment(OutputCollector *collector) const
{
  if (collector == NULL) {
    return;
  }

  if (!m_alignmentOut.str().empty()) {
    collector->Write(m_source.GetTranslationId(), m_alignmentOut.str());
  } else {
    std::vector<const Hypothesis *> edges;
    const Hypothesis *currentHypo = GetBestHypothesis();
    while (currentHypo) {
      edges.push_back(currentHypo);
      currentHypo = currentHypo->GetPrevHypo();
    }

    OutputAlignment(collector,m_source.GetTranslationId(), edges);
  }
}

void Manager::OutputAlignment(OutputCollector* collector, size_t lineNo , const vector<const Hypothesis *> &edges) const
{
  ostringstream out;
  OutputAlignment(out, edges);

  collector->Write(lineNo,out.str());
}

void Manager::OutputAlignment(ostream &out, const vector<const Hypothesis *> &edges) const
{
  size_t targetOffset = 0;

  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    const TargetPhrase &tp = edge.GetCurrTargetPhrase();
    size_t sourceOffset = edge.GetCurrSourceWordsRange().GetStartPos();

    OutputAlignment(out, tp.GetAlignTerm(), sourceOffset, targetOffset);

    targetOffset += tp.GetSize();
  }
  // Removing std::endl here breaks -alignment-output-file, so stop doing that, please :)
  // Or fix it somewhere else.
  out << std::endl;
}

void Manager::OutputDetailedTranslationReport(OutputCollector *collector) const
{
  if (collector) {
    ostringstream out;
    FixPrecision(out,PRECISION);
    TranslationAnalysis::PrintTranslationAnalysis(out, GetBestHypothesis());
    collector->Write(m_source.GetTranslationId(),out.str());
  }

}

void Manager::OutputUnknowns(OutputCollector *collector) const
{
  if (collector) {
    long translationId = m_source.GetTranslationId();
    const vector<const Phrase*>& unknowns = m_transOptColl->GetUnknownSources();
    ostringstream out;
    for (size_t i = 0; i < unknowns.size(); ++i) {
      out << *(unknowns[i]);
    }
    out << endl;
    collector->Write(translationId, out.str());
  }

}

void Manager::OutputWordGraph(OutputCollector *collector) const
{
  if (collector) {
    long translationId = m_source.GetTranslationId();
    ostringstream out;
    FixPrecision(out,PRECISION);
    GetWordGraph(translationId, out);
    collector->Write(translationId, out.str());
  }
}

void Manager::OutputSearchGraph(OutputCollector *collector) const
{
  if (collector) {
    long translationId = m_source.GetTranslationId();
    ostringstream out;
    FixPrecision(out,PRECISION);
    OutputSearchGraph(translationId, out);
    collector->Write(translationId, out.str());

#ifdef HAVE_PROTOBUF
    const StaticData &staticData = StaticData::Instance();
    if (staticData.GetOutputSearchGraphPB()) {
      ostringstream sfn;
      sfn << staticData.GetParam("output-search-graph-pb")[0] << '/' << translationId << ".pb" << ends;
      string fn = sfn.str();
      VERBOSE(2, "Writing search graph to " << fn << endl);
      fstream output(fn.c_str(), ios::trunc | ios::binary | ios::out);
      SerializeSearchGraphPB(translationId, output);
    }
#endif
  }

}

void Manager::OutputSearchGraphSLF() const
{
  const StaticData &staticData = StaticData::Instance();
  long translationId = m_source.GetTranslationId();

  // Output search graph in HTK standard lattice format (SLF)
  bool slf = staticData.GetOutputSearchGraphSLF();
  if (slf) {
    stringstream fileName;

    string dir;
    staticData.GetParameter().SetParameter<string>(dir, "output-search-graph-slf", "");

    fileName << dir << "/" << translationId << ".slf";
    ofstream *file = new ofstream;
    file->open(fileName.str().c_str());
    if (file->is_open() && file->good()) {
      ostringstream out;
      FixPrecision(out,PRECISION);
      OutputSearchGraphAsSLF(translationId, out);
      *file << out.str();
      file -> flush();
    } else {
      TRACE_ERR("Cannot output HTK standard lattice for line " << translationId << " because the output file is not open or not ready for writing" << endl);
    }
    delete file;
  }

}

void Manager::OutputSearchGraphHypergraph() const
{
  const StaticData &staticData = StaticData::Instance();
  if (staticData.GetOutputSearchGraphHypergraph()) {
    HypergraphOutput<Manager> hypergraphOutput(PRECISION);
    hypergraphOutput.Write(*this);
  }
}

void Manager::OutputLatticeMBRNBest(std::ostream& out, const vector<LatticeMBRSolution>& solutions,long translationId) const
{
  for (vector<LatticeMBRSolution>::const_iterator si = solutions.begin(); si != solutions.end(); ++si) {
    out << translationId;
    out << " |||";
    const vector<Word> mbrHypo = si->GetWords();
    for (size_t i = 0 ; i < mbrHypo.size() ; i++) {
      const Factor *factor = mbrHypo[i].GetFactor(StaticData::Instance().GetOutputFactorOrder()[0]);
      if (i>0) out << " " << *factor;
      else     out << *factor;
    }
    out << " |||";
    out << " map: " << si->GetMapScore();
    out << " w: " << mbrHypo.size();
    const vector<float>& ngramScores = si->GetNgramScores();
    for (size_t i = 0; i < ngramScores.size(); ++i) {
      out << " " << ngramScores[i];
    }
    out << " ||| " << si->GetScore();

    out << endl;
  }
}

void Manager::OutputBestHypo(const std::vector<Word>&  mbrBestHypo, long /*translationId*/, char /*reportSegmentation*/, bool /*reportAllFactors*/, ostream& out) const
{

  for (size_t i = 0 ; i < mbrBestHypo.size() ; i++) {
    const Factor *factor = mbrBestHypo[i].GetFactor(StaticData::Instance().GetOutputFactorOrder()[0]);
    UTIL_THROW_IF2(factor == NULL,
                   "No factor 0 at position " << i);
    if (i>0) out << " " << *factor;
    else     out << *factor;
  }
  out << endl;
}

void Manager::OutputBestHypo(const Moses::TrellisPath &path, long /*translationId*/, char reportSegmentation, bool reportAllFactors, std::ostream &out) const
{
  const std::vector<const Hypothesis *> &edges = path.GetEdges();

  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    OutputSurface(out, edge, StaticData::Instance().GetOutputFactorOrder(), reportSegmentation, reportAllFactors);
  }
  out << endl;
}

void Manager::OutputAlignment(std::ostringstream &out, const TrellisPath &path) const
{
  Hypothesis::OutputAlignment(out, path.GetEdges());
}

} // namespace
