/*
 *  LatticeMBR.cpp
 *  moses-cmd
 *
 *  Created by Abhishek Arun on 26/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "LatticeMBR.h"
#include "StaticData.h"
#include <algorithm>
#include <set>

using namespace std;
using namespace Moses;

namespace MosesCmd
{

size_t bleu_order = 4;
float UNKNGRAMLOGPROB = -20;
void GetOutputWords(const TrellisPath &path, vector <Word> &translation)
{
  const std::vector<const Hypothesis *> &edges = path.GetEdges();

  // print the surface factor of the translation
  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    const Phrase &phrase = edge.GetCurrTargetPhrase();
    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      translation.push_back(phrase.GetWord(pos));
    }
  }
}


void extract_ngrams(const vector<Word >& sentence, map < Phrase, int >  & allngrams)
{
  for (int k = 0; k < (int)bleu_order; k++) {
    for(int i =0; i < max((int)sentence.size()-k,0); i++) {
      Phrase ngram( k+1);
      for ( int j = i; j<= i+k; j++) {
        ngram.AddWord(sentence[j]);
      }
      ++allngrams[ngram];
    }
  }
}



void NgramScores::addScore(const Hypothesis* node, const Phrase& ngram, float score)
{
  set<Phrase>::const_iterator ngramIter = m_ngrams.find(ngram);
  if (ngramIter == m_ngrams.end()) {
    ngramIter = m_ngrams.insert(ngram).first;
  }
  map<const Phrase*,float>& ngramScores = m_scores[node];
  map<const Phrase*,float>::iterator scoreIter = ngramScores.find(&(*ngramIter));
  if (scoreIter == ngramScores.end()) {
    ngramScores[&(*ngramIter)] = score;
  } else {
    ngramScores[&(*ngramIter)] = log_sum(score,scoreIter->second);
  }
}

NgramScores::NodeScoreIterator NgramScores::nodeBegin(const Hypothesis* node)
{
  return m_scores[node].begin();
}


NgramScores::NodeScoreIterator NgramScores::nodeEnd(const Hypothesis* node)
{
  return m_scores[node].end();
}

LatticeMBRSolution::LatticeMBRSolution(const TrellisPath& path, bool isMap) :
  m_score(0.0f)
{
  const std::vector<const Hypothesis *> &edges = path.GetEdges();

  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    const Phrase &phrase = edge.GetCurrTargetPhrase();
    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      m_words.push_back(phrase.GetWord(pos));
    }
  }
  if (isMap) {
    m_mapScore = path.GetTotalScore();
  } else {
    m_mapScore = 0;
  }
}


void LatticeMBRSolution::CalcScore(map<Phrase, float>& finalNgramScores, const vector<float>& thetas, float mapWeight)
{
  m_ngramScores.assign(thetas.size()-1, -10000);

  map < Phrase, int > counts;
  extract_ngrams(m_words,counts);

  //Now score this translation
  m_score = thetas[0] * m_words.size();

  //Calculate the ngramScores, working in log space at first
  for (map < Phrase, int >::iterator ngrams = counts.begin(); ngrams != counts.end(); ++ngrams) {
    float ngramPosterior = UNKNGRAMLOGPROB;
    map<Phrase,float>::const_iterator ngramPosteriorIt = finalNgramScores.find(ngrams->first);
    if (ngramPosteriorIt != finalNgramScores.end()) {
      ngramPosterior = ngramPosteriorIt->second;
    }
    size_t ngramSize = ngrams->first.GetSize();
    m_ngramScores[ngramSize-1] = log_sum(log((float)ngrams->second) + ngramPosterior,m_ngramScores[ngramSize-1]);
  }

  //convert from log to probability and create weighted sum
  for (size_t i = 0; i < m_ngramScores.size(); ++i) {
    m_ngramScores[i] = exp(m_ngramScores[i]);
    m_score += thetas[i+1] * m_ngramScores[i];
  }


  //The map score
  m_score += m_mapScore*mapWeight;
}


void pruneLatticeFB(Lattice & connectedHyp, map < const Hypothesis*, set <const Hypothesis* > > & outgoingHyps, map<const Hypothesis*, vector<Edge> >& incomingEdges,
                    const vector< float> & estimatedScores, const Hypothesis* bestHypo, size_t edgeDensity, float scale)
{

  //Need hyp 0 in connectedHyp - Find empty hypothesis
  VERBOSE(2,"Pruning lattice to edge density " << edgeDensity << endl);
  const Hypothesis* emptyHyp = connectedHyp.at(0);
  while (emptyHyp->GetId() != 0) {
    emptyHyp = emptyHyp->GetPrevHypo();
  }
  connectedHyp.push_back(emptyHyp); //Add it to list of hyps

  //Need hyp 0's outgoing Hyps
  for (size_t i = 0; i < connectedHyp.size(); ++i) {
    if (connectedHyp[i]->GetId() > 0 && connectedHyp[i]->GetPrevHypo()->GetId() == 0)
      outgoingHyps[emptyHyp].insert(connectedHyp[i]);
  }

  //sort hyps based on estimated scores - do so by copying to multimap
  multimap<float, const Hypothesis*> sortHypsByVal;
  for (size_t i =0; i < estimatedScores.size(); ++i) {
    sortHypsByVal.insert(make_pair(estimatedScores[i], connectedHyp[i]));
  }

  multimap<float, const Hypothesis*>::const_iterator it = --sortHypsByVal.end();
  float bestScore = it->first;
  //store best score as score of hyp 0
  sortHypsByVal.insert(make_pair(bestScore, emptyHyp));


  IFVERBOSE(3) {
    for (multimap<float, const Hypothesis*>::const_iterator it = --sortHypsByVal.end(); it != --sortHypsByVal.begin(); --it) {
      const Hypothesis* currHyp =  it->second;
      cerr << "Hyp " << currHyp->GetId() << ", estimated score: " << it->first << endl;
    }
  }


  set <const Hypothesis*> survivingHyps; //store hyps that make the cut in this

  VERBOSE(2, "BEST HYPO TARGET LENGTH : " << bestHypo->GetSize() << endl)
  size_t numEdgesTotal = edgeDensity * bestHypo->GetSize(); //as per Shankar, aim for (density * target length of MAP solution) arcs
  size_t numEdgesCreated = 0;
  VERBOSE(2, "Target edge count: " << numEdgesTotal << endl);

  float prevScore = -999999;

  //now iterate over multimap
  for (multimap<float, const Hypothesis*>::const_iterator it = --sortHypsByVal.end(); it != --sortHypsByVal.begin(); --it) {
    float currEstimatedScore = it->first;
    const Hypothesis* currHyp =  it->second;

    if (numEdgesCreated >= numEdgesTotal && prevScore > currEstimatedScore) //if this hyp has equal estimated score to previous, include its edges too
      break;

    prevScore = currEstimatedScore;
    VERBOSE(3, "Num edges created : "<< numEdgesCreated << ", numEdges wanted " << numEdgesTotal << endl)
    VERBOSE(3, "Considering hyp " << currHyp->GetId() << ", estimated score: " << it->first << endl)

    survivingHyps.insert(currHyp); //CurrHyp made the cut

    // is its best predecessor already included ?
    if (survivingHyps.find(currHyp->GetPrevHypo()) != survivingHyps.end()) { //yes, then add an edge
      vector <Edge>& edges = incomingEdges[currHyp];
      Edge winningEdge(currHyp->GetPrevHypo(),currHyp,scale*(currHyp->GetScore() - currHyp->GetPrevHypo()->GetScore()),currHyp->GetCurrTargetPhrase());
      edges.push_back(winningEdge);
      ++numEdgesCreated;
    }

    //let's try the arcs too
    const ArcList *arcList = currHyp->GetArcList();
    if (arcList != NULL) {
      ArcList::const_iterator iterArcList;
      for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
        const Hypothesis *loserHypo = *iterArcList;
        const Hypothesis* loserPrevHypo = loserHypo->GetPrevHypo();
        if (survivingHyps.find(loserPrevHypo) != survivingHyps.end()) { //found it, add edge
          double arcScore = loserHypo->GetScore() - loserPrevHypo->GetScore();
          Edge losingEdge(loserPrevHypo, currHyp, arcScore*scale, loserHypo->GetCurrTargetPhrase());
          vector <Edge>& edges = incomingEdges[currHyp];
          edges.push_back(losingEdge);
          ++numEdgesCreated;
        }
      }
    }

    //Now if a successor node has already been visited, add an edge connecting the two
    map < const Hypothesis*, set < const Hypothesis* > >::const_iterator outgoingIt = outgoingHyps.find(currHyp);

    if (outgoingIt != outgoingHyps.end()) {//currHyp does have successors
      const set<const Hypothesis*> & outHyps = outgoingIt->second; //the successors
      for (set<const Hypothesis*>::const_iterator outHypIts = outHyps.begin(); outHypIts != outHyps.end(); ++outHypIts) {
        const Hypothesis* succHyp = *outHypIts;

        if (survivingHyps.find(succHyp) == survivingHyps.end()) //Have we encountered the successor yet?
          continue; //No, move on to next

        //Curr Hyp can be : a) the best predecessor  of succ b) or an arc attached to succ
        if (succHyp->GetPrevHypo() == currHyp) { //best predecessor
          vector <Edge>& succEdges = incomingEdges[succHyp];
          Edge succWinningEdge(currHyp, succHyp, scale*(succHyp->GetScore() - currHyp->GetScore()), succHyp->GetCurrTargetPhrase());
          succEdges.push_back(succWinningEdge);
          survivingHyps.insert(succHyp);
          ++numEdgesCreated;
        }

        //now, let's find an arc
        const ArcList *arcList = succHyp->GetArcList();
        if (arcList != NULL) {
          ArcList::const_iterator iterArcList;
          //QUESTION: What happens if there's more than one loserPrevHypo?
          for (iterArcList = arcList->begin() ; iterArcList != arcList->end() ; ++iterArcList) {
            const Hypothesis *loserHypo = *iterArcList;
            const Hypothesis* loserPrevHypo = loserHypo->GetPrevHypo();
            if (loserPrevHypo == currHyp) { //found it
              vector <Edge>& succEdges = incomingEdges[succHyp];
              double arcScore = loserHypo->GetScore() - currHyp->GetScore();
              Edge losingEdge(currHyp, succHyp,scale* arcScore, loserHypo->GetCurrTargetPhrase());
              succEdges.push_back(losingEdge);
              ++numEdgesCreated;
            }
          }
        }
      }
    }
  }

  connectedHyp.clear();
  for (set <const Hypothesis*>::iterator it =  survivingHyps.begin(); it != survivingHyps.end(); ++it) {
    connectedHyp.push_back(*it);
  }

  VERBOSE(2, "Done! Num edges created : "<< numEdgesCreated << ", numEdges wanted " << numEdgesTotal << endl)

  IFVERBOSE(3) {
    cerr << "Surviving hyps: " ;
    for (set <const Hypothesis*>::iterator it =  survivingHyps.begin(); it != survivingHyps.end(); ++it) {
      cerr << (*it)->GetId() << " ";
    }
    cerr << endl;
  }


}

void calcNgramExpectations(Lattice & connectedHyp, map<const Hypothesis*, vector<Edge> >& incomingEdges,
                           map<Phrase, float>& finalNgramScores, bool posteriors)
{

  sort(connectedHyp.begin(),connectedHyp.end(),ascendingCoverageCmp); //sort by increasing source word cov

  /*cerr << "Lattice:" << endl;
  for (Lattice::const_iterator i = connectedHyp.begin(); i != connectedHyp.end(); ++i) {
      const Hypothesis* h = *i;
      cerr << *h << endl;
      const vector<Edge>& edges = incomingEdges[h];
      for (size_t e = 0; e < edges.size(); ++e) {
          cerr << edges[e];
      }
  }*/

  map<const Hypothesis*, float> forwardScore;
  forwardScore[connectedHyp[0]] = 0.0f; //forward score of hyp 0 is 1 (or 0 in logprob space)
  set< const Hypothesis *> finalHyps; //store completed hyps

  NgramScores ngramScores;//ngram scores for each hyp

  for (size_t i = 1; i < connectedHyp.size(); ++i) {
    const Hypothesis* currHyp = connectedHyp[i];
    if (currHyp->GetWordsBitmap().IsComplete()) {
      finalHyps.insert(currHyp);
    }

    VERBOSE(3, "Processing hyp: " << currHyp->GetId() << ", num words cov= " << currHyp->GetWordsBitmap().GetNumWordsCovered() <<  endl)

    vector <Edge> & edges = incomingEdges[currHyp];
    for (size_t e = 0; e < edges.size(); ++e) {
      const Edge& edge = edges[e];
      if (forwardScore.find(currHyp) == forwardScore.end()) {
        forwardScore[currHyp] = forwardScore[edge.GetTailNode()] + edge.GetScore();
        VERBOSE(3, "Fwd score["<<currHyp->GetId()<<"] = fwdScore["<<edge.GetTailNode()->GetId() << "] + edge Score: " << edge.GetScore() << endl)
      } else {
        forwardScore[currHyp] = log_sum(forwardScore[currHyp], forwardScore[edge.GetTailNode()] + edge.GetScore());
        VERBOSE(3, "Fwd score["<<currHyp->GetId()<<"] += fwdScore["<<edge.GetTailNode()->GetId() << "] + edge Score: " << edge.GetScore() << endl)
      }
    }

    //Process ngrams now
    for (size_t j =0 ; j < edges.size(); ++j) {
      Edge& edge = edges[j];
      const NgramHistory & incomingPhrases = edge.GetNgrams(incomingEdges);

      //let's first score ngrams introduced by this edge
      for (NgramHistory::const_iterator it = incomingPhrases.begin(); it != incomingPhrases.end(); ++it) {
        const Phrase& ngram = it->first;
        const PathCounts& pathCounts = it->second;
        VERBOSE(4, "Calculating score for: " << it->first << endl)

        for (PathCounts::const_iterator pathCountIt = pathCounts.begin(); pathCountIt != pathCounts.end(); ++pathCountIt) {
          //Score of an n-gram is forward score of head node of leftmost edge + all edge scores
          const Path&  path = pathCountIt->first;
          //cerr << "path count for " << ngram << " is " << pathCountIt->second << endl;
          float score = forwardScore[path[0]->GetTailNode()];
          for (size_t i = 0; i < path.size(); ++i) {
            score += path[i]->GetScore();
          }
          //if we're doing expectations, then the number of times the ngram
          //appears on the path is relevant.
          size_t count = posteriors ? 1 : pathCountIt->second;
          for (size_t k = 0; k < count; ++k) {
            ngramScores.addScore(currHyp,ngram,score);
          }
        }
      }

      //Now score ngrams that are just being propagated from the history
      for (NgramScores::NodeScoreIterator it = ngramScores.nodeBegin(edge.GetTailNode());
           it != ngramScores.nodeEnd(edge.GetTailNode()); ++it) {
        const Phrase & currNgram = *(it->first);
        float currNgramScore = it->second;
        VERBOSE(4, "Calculating score for: " << currNgram << endl)

        // For posteriors, don't double count ngrams
        if (!posteriors || incomingPhrases.find(currNgram) == incomingPhrases.end()) {
          float score = edge.GetScore() + currNgramScore;
          ngramScores.addScore(currHyp,currNgram,score);
        }
      }

    }
  }

  float Z = 9999999; //the total score of the lattice

  //Done - Print out ngram posteriors for final hyps
  for (set< const Hypothesis *>::iterator finalHyp = finalHyps.begin(); finalHyp != finalHyps.end(); ++finalHyp) {
    const Hypothesis* hyp = *finalHyp;

    for (NgramScores::NodeScoreIterator it = ngramScores.nodeBegin(hyp); it != ngramScores.nodeEnd(hyp); ++it) {
      const Phrase& ngram = *(it->first);
      if (finalNgramScores.find(ngram) == finalNgramScores.end()) {
        finalNgramScores[ngram] = it->second;
      } else {
        finalNgramScores[ngram] = log_sum(it->second,  finalNgramScores[ngram]);
      }
    }

    if (Z == 9999999) {
      Z = forwardScore[hyp];
    } else {
      Z = log_sum(Z, forwardScore[hyp]);
    }
  }

  //Z *= scale;  //scale the score

  for (map<Phrase, float>::iterator finalScoresIt = finalNgramScores.begin();  finalScoresIt != finalNgramScores.end(); ++finalScoresIt) {
    finalScoresIt->second =  finalScoresIt->second - Z;
    IFVERBOSE(2) {
      VERBOSE(2,finalScoresIt->first << " [" << finalScoresIt->second << "]" << endl);
    }
  }

}

const NgramHistory& Edge::GetNgrams(map<const Hypothesis*, vector<Edge> > & incomingEdges)
{

  if (m_ngrams.size() > 0)
    return m_ngrams;

  const Phrase& currPhrase = GetWords();
  //Extract the n-grams local to this edge
  for (size_t start = 0; start < currPhrase.GetSize(); ++start) {
    for (size_t end = start; end < start + bleu_order; ++end) {
      if (end < currPhrase.GetSize()) {
        Phrase edgeNgram(end-start+1);
        for (size_t index = start; index <= end; ++index) {
          edgeNgram.AddWord(currPhrase.GetWord(index));
        }
        //cout << "Inserting Phrase : " << edgeNgram << endl;
        vector<const Edge*> edgeHistory;
        edgeHistory.push_back(this);
        storeNgramHistory(edgeNgram, edgeHistory);
      } else {
        break;
      }
    }
  }

  map<const Hypothesis*, vector<Edge> >::iterator it = incomingEdges.find(m_tailNode);
  if (it != incomingEdges.end()) { //node has incoming edges
    vector<Edge> & inEdges = it->second;

    for (vector<Edge>::iterator edge = inEdges.begin(); edge != inEdges.end(); ++edge) {//add the ngrams straddling prev and curr edge
      const NgramHistory & edgeIncomingNgrams = edge->GetNgrams(incomingEdges);
      for (NgramHistory::const_iterator edgeInNgramHist = edgeIncomingNgrams.begin(); edgeInNgramHist != edgeIncomingNgrams.end(); ++edgeInNgramHist) {
        const Phrase& edgeIncomingNgram = edgeInNgramHist->first;
        const PathCounts &  edgeIncomingNgramPaths = edgeInNgramHist->second;
        size_t back = min(edgeIncomingNgram.GetSize(), edge->GetWordsSize());
        const Phrase&  edgeWords = edge->GetWords();
        IFVERBOSE(3) {
          cerr << "Edge: "<< *edge <<endl;
          cerr << "edgeWords: " << edgeWords << endl;
          cerr << "edgeInNgram: " << edgeIncomingNgram << endl;
        }

        Phrase edgeSuffix(ARRAY_SIZE_INCR);
        Phrase ngramSuffix(ARRAY_SIZE_INCR);
        GetPhraseSuffix(edgeWords,back,edgeSuffix);
        GetPhraseSuffix(edgeIncomingNgram,back,ngramSuffix);

        if (ngramSuffix == edgeSuffix) { //we've got the suffix of previous edge
          size_t  edgeInNgramSize =  edgeIncomingNgram.GetSize();

          for (size_t i = 0; i < GetWordsSize() && i + edgeInNgramSize < bleu_order ; ++i) {
            Phrase newNgram(edgeIncomingNgram);
            for (size_t j = 0; j <= i ; ++j) {
              newNgram.AddWord(GetWords().GetWord(j));
            }
            VERBOSE(3, "Inserting New Phrase : " << newNgram << endl)

            for (PathCounts::const_iterator pathIt = edgeIncomingNgramPaths.begin(); pathIt !=  edgeIncomingNgramPaths.end(); ++pathIt) {
              Path newNgramPath = pathIt->first;
              newNgramPath.push_back(this);
              storeNgramHistory(newNgram, newNgramPath, pathIt->second);
            }
          }
        }
      }
    }
  }
  return m_ngrams;
}

//Add the last lastN words of origPhrase to targetPhrase
void Edge::GetPhraseSuffix(const Phrase&  origPhrase, size_t lastN, Phrase& targetPhrase) const
{
  size_t origSize = origPhrase.GetSize();
  size_t startIndex = origSize - lastN;
  for (size_t index = startIndex; index < origPhrase.GetSize(); ++index) {
    targetPhrase.AddWord(origPhrase.GetWord(index));
  }
}

bool Edge::operator< (const Edge& compare ) const
{
  if (m_headNode->GetId() < compare.m_headNode->GetId())
    return true;
  if (compare.m_headNode->GetId() < m_headNode->GetId())
    return false;
  if (m_tailNode->GetId() < compare.m_tailNode->GetId())
    return true;
  if (compare.m_tailNode->GetId() < m_tailNode->GetId())
    return false;
  return GetScore() <  compare.GetScore();
}

ostream& operator<< (ostream& out, const Edge& edge)
{
  out << "Head: " << edge.m_headNode->GetId() << ", Tail: " << edge.m_tailNode->GetId() << ", Score: " << edge.m_score << ", Phrase: " << edge.m_targetPhrase << endl;
  return out;
}

bool ascendingCoverageCmp(const Hypothesis* a, const Hypothesis* b)
{
  return a->GetWordsBitmap().GetNumWordsCovered() <  b->GetWordsBitmap().GetNumWordsCovered();
}

void getLatticeMBRNBest(Manager& manager, TrellisPathList& nBestList,
                        vector<LatticeMBRSolution>& solutions, size_t n)
{
  const StaticData& staticData = StaticData::Instance();
  std::map < int, bool > connected;
  std::vector< const Hypothesis *> connectedList;
  map<Phrase, float> ngramPosteriors;
  std::map < const Hypothesis*, set <const Hypothesis*> > outgoingHyps;
  map<const Hypothesis*, vector<Edge> > incomingEdges;
  vector< float> estimatedScores;
  manager.GetForwardBackwardSearchGraph(&connected, &connectedList, &outgoingHyps, &estimatedScores);
  pruneLatticeFB(connectedList, outgoingHyps, incomingEdges, estimatedScores, manager.GetBestHypothesis(), staticData.GetLatticeMBRPruningFactor(),staticData.GetMBRScale());
  calcNgramExpectations(connectedList, incomingEdges, ngramPosteriors,true);

  vector<float> mbrThetas = staticData.GetLatticeMBRThetas();
  float p = staticData.GetLatticeMBRPrecision();
  float r = staticData.GetLatticeMBRPRatio();
  float mapWeight = staticData.GetLatticeMBRMapWeight();
  if (mbrThetas.size() == 0) { //thetas not specified on the command line, use p and r instead
    mbrThetas.push_back(-1); //Theta 0
    mbrThetas.push_back(1/(bleu_order*p));
    for (size_t i = 2; i <= bleu_order; ++i) {
      mbrThetas.push_back(mbrThetas[i-1] / r);
    }
  }
  IFVERBOSE(2) {
    VERBOSE(2,"Thetas: ");
    for (size_t i = 0; i < mbrThetas.size(); ++i) {
      VERBOSE(2,mbrThetas[i] << " ");
    }
    VERBOSE(2,endl);
  }
  TrellisPathList::const_iterator iter;
  size_t ctr = 0;
  LatticeMBRSolutionComparator comparator;
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter, ++ctr) {
    const TrellisPath &path = **iter;
    solutions.push_back(LatticeMBRSolution(path,iter==nBestList.begin()));
    solutions.back().CalcScore(ngramPosteriors,mbrThetas,mapWeight);
    sort(solutions.begin(), solutions.end(), comparator);
    while (solutions.size() > n) {
      solutions.pop_back();
    }
  }
  VERBOSE(2,"LMBR Score: " << solutions[0].GetScore() << endl);
}

vector<Word> doLatticeMBR(Manager& manager, TrellisPathList& nBestList)
{

  vector<LatticeMBRSolution> solutions;
  getLatticeMBRNBest(manager, nBestList, solutions,1);
  return solutions.at(0).GetWords();
}

const TrellisPath doConsensusDecoding(Manager& manager, TrellisPathList& nBestList)
{
  static const int BLEU_ORDER = 4;
  static const float SMOOTH = 1;

  //calculate the ngram expectations
  const StaticData& staticData = StaticData::Instance();
  std::map < int, bool > connected;
  std::vector< const Hypothesis *> connectedList;
  map<Phrase, float> ngramExpectations;
  std::map < const Hypothesis*, set <const Hypothesis*> > outgoingHyps;
  map<const Hypothesis*, vector<Edge> > incomingEdges;
  vector< float> estimatedScores;
  manager.GetForwardBackwardSearchGraph(&connected, &connectedList, &outgoingHyps, &estimatedScores);
  pruneLatticeFB(connectedList, outgoingHyps, incomingEdges, estimatedScores, manager.GetBestHypothesis(), staticData.GetLatticeMBRPruningFactor(),staticData.GetMBRScale());
  calcNgramExpectations(connectedList, incomingEdges, ngramExpectations,false);

  //expected length is sum of expected unigram counts
  //cerr << "Thread " << pthread_self() <<  " Ngram expectations size: " << ngramExpectations.size() << endl;
  float ref_length = 0.0f;
  for (map<Phrase,float>::const_iterator ref_iter = ngramExpectations.begin();
       ref_iter != ngramExpectations.end(); ++ref_iter) {
    //cerr << "Ngram: " << ref_iter->first << " score: " <<
    //    ref_iter->second << endl;
    if (ref_iter->first.GetSize() == 1) {
      ref_length += exp(ref_iter->second);
      //    cerr << "Expected for " << ref_iter->first << " is " << exp(ref_iter->second) << endl;
    }
  }

  VERBOSE(2,"REF Length: " << ref_length << endl);

  //use the ngram expectations to rescore the nbest list.
  TrellisPathList::const_iterator iter;
  TrellisPathList::const_iterator best = nBestList.end();
  float bestScore = -100000;
  //cerr << "nbest list size: " << nBestList.GetSize() << endl;
  for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter) {
    const TrellisPath &path = **iter;
    vector<Word> words;
    map<Phrase,int> ngrams;
    GetOutputWords(path,words);
    /*for (size_t i = 0; i < words.size(); ++i) {
        cerr << words[i].GetFactor(0)->GetString() << " ";
    }
    cerr << endl;
    */
    extract_ngrams(words,ngrams);

    vector<float> comps(2*BLEU_ORDER+1);
    float logbleu = 0.0;
    float brevity = 0.0;
    int hyp_length = words.size();
    for (int i = 0; i < BLEU_ORDER; ++i) {
      comps[2*i] = 0.0;
      comps[2*i+1] = max(hyp_length-i,0);
    }

    for (map<Phrase,int>::const_iterator hyp_iter = ngrams.begin();
         hyp_iter != ngrams.end(); ++hyp_iter) {
      map<Phrase,float>::const_iterator ref_iter = ngramExpectations.find(hyp_iter->first);
      if (ref_iter != ngramExpectations.end()) {
        comps[2*(hyp_iter->first.GetSize()-1)] += min(exp(ref_iter->second), (float)(hyp_iter->second));
      }

    }
    comps[comps.size()-1] = ref_length;
    /*for (size_t i = 0; i < comps.size(); ++i) {
        cerr << comps[i] << " ";
    }
    cerr << endl;
    */

    float score = 0.0f;
    if (comps[0] != 0) {
      for (int i=0; i<BLEU_ORDER; i++) {
        if ( i > 0 ) {
          logbleu += log((float)comps[2*i]+SMOOTH)-log((float)comps[2*i+1]+SMOOTH);
        } else {
          logbleu += log((float)comps[2*i])-log((float)comps[2*i+1]);
        }
      }
      logbleu /= BLEU_ORDER;
      brevity = 1.0-(float)comps[comps.size()-1]/comps[1]; // comps[comps_n-1] is the ref length, comps[1] is the test length
      if (brevity < 0.0) {
        logbleu += brevity;
      }
      score =  exp(logbleu);
    }

    //cerr << "score: " << score << " bestScore: " << bestScore <<  endl;
    if (score > bestScore) {
      bestScore = score;
      best = iter;
      VERBOSE(2,"NEW BEST: " << score << endl);
      //for (size_t i = 0; i < comps.size(); ++i) {
      //    cerr << comps[i] << " ";
      //}
      //cerr << endl;
    }
  }

  assert (best != nBestList.end());
  return **best;
  //vector<Word> bestWords;
  //GetOutputWords(**best,bestWords);
  //return bestWords;
}

}


