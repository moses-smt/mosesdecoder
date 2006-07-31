// $Id$

#ifndef _SCORE_INDEX_MANAGER_H_
#define _SCORE_INDEX_MANAGER_H_

#include <iostream>
#include <vector>

class ScoreProducer;

class ScoreIndexManager
{
  friend std::ostream& operator<<(std::ostream& os, const ScoreIndexManager& sim);
public:
	ScoreIndexManager() : m_last(0) {}
	void AddScoreProducer(const ScoreProducer* producer);
	size_t GetBeginIndex(size_t scoreBookkeepingID) const { return m_begins[scoreBookkeepingID]; }
	size_t GetEndIndex(size_t scoreBookkeepingID) const { return m_ends[scoreBookkeepingID]; }
	size_t GetTotalNumberOfScores() const { return m_last; }

private:
	ScoreIndexManager(const ScoreIndexManager&); // don't implement

	std::vector<size_t> m_begins;
	std::vector<size_t> m_ends;
	std::vector<const ScoreProducer*> m_producers;
	size_t m_last;
};

#endif
