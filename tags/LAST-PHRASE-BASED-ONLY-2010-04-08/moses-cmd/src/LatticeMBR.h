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

template<class T>
T log_sum (T log_a, T log_b)
{
  T v;
  if (log_a < log_b) {
    v = log_b+log ( 1 + exp ( log_a-log_b ));
  } else {
    v = log_a+log ( 1 + exp ( log_b-log_a ));
  }
  return ( v );
}

class Edge;

typedef std::vector< const Hypothesis *> Lattice;
typedef vector<const Edge*> Path; 
typedef map<Path, size_t> PathCounts;
typedef map<Phrase, PathCounts > NgramHistory;

class Edge {
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
  
  friend ostream& operator<< (ostream& out, const Edge& edge); 
  
  const NgramHistory&  GetNgrams(  map<const Hypothesis*, vector<Edge> > & incomingEdges) ;
  
  bool operator < (const Edge & compare) const;
  
  void GetPhraseSuffix(const Phrase& origPhrase, size_t lastN, Phrase& targetPhrase) const;  
  
  void storeNgramHistory(const Phrase& phrase, Path & path, size_t count = 1){
    m_ngrams[phrase][path]+= count; 
  }
  
};

/**
* Data structure to hold the ngram scores as we traverse the lattice. Maps (hypo,ngram) to score
*/
class NgramScores {
    public:
        NgramScores() {}
        
        /** logsum this score to the existing score */
        void addScore(const Hypothesis* node, const Phrase& ngram, float score);
        
        /** Iterate through ngrams for selected node */
        typedef map<const Phrase*, float>::const_iterator NodeScoreIterator;
        NodeScoreIterator nodeBegin(const Hypothesis* node);
        NodeScoreIterator nodeEnd(const Hypothesis* node);
        
    private:
        set<Phrase> m_ngrams;
        map<const Hypothesis*, map<const Phrase*, float> > m_scores;
};


/** Holds a lattice mbr solution, and its scores */
class LatticeMBRSolution {
    public:
        /** Read the words from the path */
        LatticeMBRSolution(const TrellisPath& path, bool isMap);
        const vector<float>& GetNgramScores() const {return m_ngramScores;}
        const vector<Word>& GetWords() const {return m_words;}
        float GetMapScore() const {return m_mapScore;}
        float GetScore() const {return m_score;}
        
        /** Initialise ngram scores */
        void CalcScore(map<Phrase, float>& finalNgramScores, const vector<float>& thetas, float mapWeight);
    
    private:
        vector<Word> m_words;
        float m_mapScore;
        vector<float> m_ngramScores;
        float m_score;
};

struct LatticeMBRSolutionComparator {
    bool operator()(const LatticeMBRSolution& a, const LatticeMBRSolution& b) {
        return a.GetScore() > b.GetScore();
    }
};

void pruneLatticeFB(Lattice & connectedHyp, map < const Hypothesis*, set <const Hypothesis* > > & outgoingHyps, map<const Hypothesis*, vector<Edge> >& incomingEdges, 
                    const vector< float> & estimatedScores, const Hypothesis*, size_t edgeDensity);

//Use the ngram scores to rerank the nbest list, return at most n solutions
void getLatticeMBRNBest(Manager& manager, TrellisPathList& nBestList, vector<LatticeMBRSolution>& solutions, size_t n);
//calculate expectated ngram counts, clipping at 1 (ie calculating posteriors) if posteriors==true.
void calcNgramExpectations(Lattice & connectedHyp, map<const Hypothesis*, vector<Edge> >& incomingEdges, float scale, map<Phrase,
                            float>& finalNgramScores, bool posteriors);
void GetOutputFactors(const TrellisPath &path, vector <Word> &translation);
void extract_ngrams(const vector<Word >& sentence, map < Phrase, int >  & allngrams);
bool ascendingCoverageCmp(const Hypothesis* a, const Hypothesis* b);
vector<Word> doLatticeMBR(Manager& manager, TrellisPathList& nBestList);
vector<Word> doConsensusDecoding(Manager& manager, TrellisPathList& nBestList);
#endif
