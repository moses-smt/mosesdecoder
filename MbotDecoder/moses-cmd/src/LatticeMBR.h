/*
 *  LatticeMBR.h
 *  moses-cmd
 *
 *  Created by Abhishek Arun on 26/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef moses_cmd_LatticeMBR_h
#define moses_cmd_LatticeMBR_h

#include <map>
#include <vector>
#include <set>
#include "Hypothesis.h"
#include "Manager.h"
#include "TrellisPathList.h"

using namespace Moses;



class Edge;

typedef std::vector< const Hypothesis *> Lattice;
typedef std::vector<const Edge*> Path;
typedef std::map<Path, size_t> PathCounts;
typedef std::map<Phrase, PathCounts > NgramHistory;

class Edge
{
  const Hypothesis* m_tailNode;
  const Hypothesis* m_headNode;
  float m_score;
  TargetPhrase m_targetPhrase;
  NgramHistory m_ngrams;

public:
  Edge(const Hypothesis* from, const Hypothesis* to, float score, const TargetPhrase& targetPhrase) : m_tailNode(from), m_headNode(to), m_score(score), m_targetPhrase(targetPhrase) {
    //cout << "Creating new edge from Node " << from->GetId() << ", to Node : " << to->GetId() << ", score: " << score << " phrase: " << targetPhrase << endl;
  }

  const Hypothesis* GetHeadNode() const {
    return m_headNode;
  }

  const Hypothesis* GetTailNode() const {
    return m_tailNode;
  }

  float GetScore() const {
    return m_score;
  }

  size_t GetWordsSize() const {
    return m_targetPhrase.GetSize();
  }

  const Phrase& GetWords() const {
    return m_targetPhrase;
  }

  friend std::ostream& operator<< (std::ostream& out, const Edge& edge);

  const NgramHistory&  GetNgrams(  std::map<const Hypothesis*, std::vector<Edge> > & incomingEdges) ;

  bool operator < (const Edge & compare) const;

  void GetPhraseSuffix(const Phrase& origPhrase, size_t lastN, Phrase& targetPhrase) const;

  void storeNgramHistory(const Phrase& phrase, Path & path, size_t count = 1) {
    m_ngrams[phrase][path]+= count;
  }

};

/**
* Data structure to hold the ngram scores as we traverse the lattice. Maps (hypo,ngram) to score
*/
class NgramScores
{
public:
  NgramScores() {}

  /** logsum this score to the existing score */
  void addScore(const Hypothesis* node, const Phrase& ngram, float score);

  /** Iterate through ngrams for selected node */
  typedef std::map<const Phrase*, float>::const_iterator NodeScoreIterator;
  NodeScoreIterator nodeBegin(const Hypothesis* node);
  NodeScoreIterator nodeEnd(const Hypothesis* node);

private:
  std::set<Phrase> m_ngrams;
  std::map<const Hypothesis*, std::map<const Phrase*, float> > m_scores;
};


/** Holds a lattice mbr solution, and its scores */
class LatticeMBRSolution
{
public:
  /** Read the words from the path */
  LatticeMBRSolution(const TrellisPath& path, bool isMap);
  const std::vector<float>& GetNgramScores() const {
    return m_ngramScores;
  }
  const std::vector<Word>& GetWords() const {
    return m_words;
  }
  float GetMapScore() const {
    return m_mapScore;
  }
  float GetScore() const {
    return m_score;
  }

  /** Initialise ngram scores */
  void CalcScore(std::map<Phrase, float>& finalNgramScores, const std::vector<float>& thetas, float mapWeight);

private:
  std::vector<Word> m_words;
  float m_mapScore;
  std::vector<float> m_ngramScores;
  float m_score;
};

struct LatticeMBRSolutionComparator {
  bool operator()(const LatticeMBRSolution& a, const LatticeMBRSolution& b) {
    return a.GetScore() > b.GetScore();
  }
};

void pruneLatticeFB(Lattice & connectedHyp, std::map < const Hypothesis*, std::set <const Hypothesis* > > & outgoingHyps, std::map<const Hypothesis*, std::vector<Edge> >& incomingEdges,
                    const std::vector< float> & estimatedScores, const Hypothesis*, size_t edgeDensity,float scale);

//Use the ngram scores to rerank the nbest list, return at most n solutions
void getLatticeMBRNBest(Manager& manager, TrellisPathList& nBestList, std::vector<LatticeMBRSolution>& solutions, size_t n);
//calculate expectated ngram counts, clipping at 1 (ie calculating posteriors) if posteriors==true.
void calcNgramExpectations(Lattice & connectedHyp, std::map<const Hypothesis*, std::vector<Edge> >& incomingEdges, std::map<Phrase,
                           float>& finalNgramScores, bool posteriors);
void GetOutputFactors(const TrellisPath &path, std::vector <Word> &translation);
void extract_ngrams(const std::vector<Word >& sentence, std::map < Phrase, int >  & allngrams);
bool ascendingCoverageCmp(const Hypothesis* a, const Hypothesis* b);
std::vector<Word> doLatticeMBR(Manager& manager, TrellisPathList& nBestList);
const TrellisPath doConsensusDecoding(Manager& manager, TrellisPathList& nBestList);
//std::vector<Word> doConsensusDecoding(Manager& manager, TrellisPathList& nBestList);
#endif
