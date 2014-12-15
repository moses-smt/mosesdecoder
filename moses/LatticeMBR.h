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
#include "moses/Hypothesis.h"
#include "moses/Manager.h"
#include "moses/TrellisPathList.h"



namespace Moses
{

class Edge;

typedef std::vector< const Moses::Hypothesis *> Lattice;
typedef std::vector<const Edge*> Path;
typedef std::map<Path, size_t> PathCounts;
typedef std::map<Moses::Phrase, PathCounts > NgramHistory;

class Edge
{
  const Moses::Hypothesis* m_tailNode;
  const Moses::Hypothesis* m_headNode;
  float m_score;
  Moses::TargetPhrase m_targetPhrase;
  NgramHistory m_ngrams;

public:
  Edge(const Moses::Hypothesis* from, const Moses::Hypothesis* to, float score, const Moses::TargetPhrase& targetPhrase) : m_tailNode(from), m_headNode(to), m_score(score), m_targetPhrase(targetPhrase) {
    //cout << "Creating new edge from Node " << from->GetId() << ", to Node : " << to->GetId() << ", score: " << score << " phrase: " << targetPhrase << endl;
  }

  const Moses::Hypothesis* GetHeadNode() const {
    return m_headNode;
  }

  const Moses::Hypothesis* GetTailNode() const {
    return m_tailNode;
  }

  float GetScore() const {
    return m_score;
  }

  size_t GetWordsSize() const {
    return m_targetPhrase.GetSize();
  }

  const Moses::Phrase& GetWords() const {
    return m_targetPhrase;
  }

  friend std::ostream& operator<< (std::ostream& out, const Edge& edge);

  const NgramHistory&  GetNgrams(  std::map<const Moses::Hypothesis*, std::vector<Edge> > & incomingEdges) ;

  bool operator < (const Edge & compare) const;

  void GetPhraseSuffix(const Moses::Phrase& origPhrase, size_t lastN, Moses::Phrase& targetPhrase) const;

  void storeNgramHistory(const Moses::Phrase& phrase, Path & path, size_t count = 1) {
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
  void addScore(const Moses::Hypothesis* node, const Moses::Phrase& ngram, float score);

  /** Iterate through ngrams for selected node */
  typedef std::map<const Moses::Phrase*, float>::const_iterator NodeScoreIterator;
  NodeScoreIterator nodeBegin(const Moses::Hypothesis* node);
  NodeScoreIterator nodeEnd(const Moses::Hypothesis* node);

private:
  std::set<Moses::Phrase> m_ngrams;
  std::map<const Moses::Hypothesis*, std::map<const Moses::Phrase*, float> > m_scores;
};


/** Holds a lattice mbr solution, and its scores */
class LatticeMBRSolution
{
public:
  /** Read the words from the path */
  LatticeMBRSolution(const Moses::TrellisPath& path, bool isMap);
  const std::vector<float>& GetNgramScores() const {
    return m_ngramScores;
  }
  const std::vector<Moses::Word>& GetWords() const {
    return m_words;
  }
  float GetMapScore() const {
    return m_mapScore;
  }
  float GetScore() const {
    return m_score;
  }

  /** Initialise ngram scores */
  void CalcScore(std::map<Moses::Phrase, float>& finalNgramScores, const std::vector<float>& thetas, float mapWeight);

private:
  std::vector<Moses::Word> m_words;
  float m_mapScore;
  std::vector<float> m_ngramScores;
  float m_score;
};

struct LatticeMBRSolutionComparator {
  bool operator()(const LatticeMBRSolution& a, const LatticeMBRSolution& b) {
    return a.GetScore() > b.GetScore();
  }
};

void pruneLatticeFB(Lattice & connectedHyp, std::map < const Moses::Hypothesis*, std::set <const Moses::Hypothesis* > > & outgoingHyps, std::map<const Moses::Hypothesis*, std::vector<Edge> >& incomingEdges,
                    const std::vector< float> & estimatedScores, const Moses::Hypothesis*, size_t edgeDensity,float scale);

//Use the ngram scores to rerank the nbest list, return at most n solutions
void getLatticeMBRNBest(const Moses::Manager& manager, const Moses::TrellisPathList& nBestList, std::vector<LatticeMBRSolution>& solutions, size_t n);
//calculate expectated ngram counts, clipping at 1 (ie calculating posteriors) if posteriors==true.
void calcNgramExpectations(Lattice & connectedHyp, std::map<const Moses::Hypothesis*, std::vector<Edge> >& incomingEdges, std::map<Moses::Phrase,
                           float>& finalNgramScores, bool posteriors);
void GetOutputFactors(const Moses::TrellisPath &path, std::vector <Moses::Word> &translation);
void extract_ngrams(const std::vector<Moses::Word >& sentence, std::map < Moses::Phrase, int >  & allngrams);
bool ascendingCoverageCmp(const Moses::Hypothesis* a, const Moses::Hypothesis* b);
std::vector<Moses::Word> doLatticeMBR(const Moses::Manager& manager, const Moses::TrellisPathList& nBestList);
const Moses::TrellisPath doConsensusDecoding(const Moses::Manager& manager, const Moses::TrellisPathList& nBestList);
//std::vector<Moses::Word> doConsensusDecoding(Moses::Manager& manager, Moses::TrellisPathList& nBestList);

}

#endif
