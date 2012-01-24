// $Id$

#ifndef moses_ScoreIndexManager_h
#define moses_ScoreIndexManager_h

#include <iostream>
#include <vector>
#ifdef HAVE_PROTOBUF
#include "hypergraph.pb.h"
#endif

namespace Moses
{

class ScoreProducer;
class ScoreComponentCollection;  // debugging only
class StatefulFeatureFunction;
class StatelessFeatureFunction;

/** Keep track of scores and score producers. Each score producer is reserved contiguous set of slots
	* to put their score components. All the score components are arranged in a vector with no gaps.
 * Only 1 ScoreIndexManager object should be instantiated
*/
class ScoreIndexManager
{
  friend std::ostream& operator<<(std::ostream& os, const ScoreIndexManager& sim);
public:
  ScoreIndexManager() : m_last(0) {}

  //! new score producer to manage. Producers must be inserted in the order they are created
  void AddScoreProducer(const ScoreProducer* producer);
  void InitFeatureNames();

  //! starting score index for a particular score producer with scoreBookkeepingID
  size_t GetBeginIndex(size_t scoreBookkeepingID) const {
    return m_begins[scoreBookkeepingID];
  }
  //! end score index for a particular score producer with scoreBookkeepingID
  size_t GetEndIndex(size_t scoreBookkeepingID) const {
    return m_ends[scoreBookkeepingID];
  }
  //! sum of all score components from every score producer
  size_t GetTotalNumberOfScores() const {
    return m_last;
  }
  //! print unweighted scores of each ScoreManager to stream os
  void PrintLabeledScores(std::ostream& os, const ScoreComponentCollection& scc) const;
  //! print weighted scores of each ScoreManager to stream os
  void PrintLabeledWeightedScores(std::ostream& os, const ScoreComponentCollection& scc, const std::vector<float>& weights) const;
#ifdef HAVE_PROTOBUF
  void SerializeFeatureNamesToPB(hgmert::Hypergraph* hg) const;
#endif
  void InitWeightVectorFromFile(const std::string& fnam, std::vector<float>* m_allWeights) const;
private:
  ScoreIndexManager(const ScoreIndexManager&); // don't implement

  std::vector<size_t> m_begins;
  std::vector<size_t> m_ends;
  std::vector<const ScoreProducer*> m_producers; /**< all the score producers in this run */
  std::vector<std::string> m_featureNames;
  std::vector<std::string> m_featureShortNames;
  size_t m_last;
};


}

#endif
