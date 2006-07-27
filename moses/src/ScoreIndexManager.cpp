
#include <iostream>
#include <string>
#include <assert.h>
#include "ScoreIndexManager.h"
#include "ScoreProducer.h"

void ScoreIndexManager::AddScoreProducer(const ScoreProducer* sp)
{
	// Producers must be inserted in the order they are created
  assert(m_begins.size() == (sp->GetScoreBookkeepingID() - 1));
	m_producers.push_back(sp);
  m_begins.push_back(m_last);
	size_t numScoreCompsProduced = sp->GetNumScoreComponents();
	assert(numScoreCompsProduced > 0);
	m_last += numScoreCompsProduced;
	m_ends.push_back(m_last);
	std::cerr << "AddScoreProducer(" << sp << "): id=" << sp->GetScoreBookkeepingID() << ", new last=" << m_last << std::endl;
}

std::ostream& operator<<(std::ostream& os, const ScoreIndexManager& sim)
{
	size_t cur_i = 0;
	size_t cur_scoreType = 0;
	while (cur_i < sim.m_last) {
		bool first = true;
		while (cur_i < sim.m_ends[cur_scoreType]) {
			os << "  " << (cur_i < 10 ? " " : "") << cur_i << " ";
			if (first) {
				os << sim.m_producers[cur_scoreType]->GetScoreProducerDescription()
					 << std::endl;
				first = false;
			} else {
				os << "    \"         \"" << std::endl;
			}
			cur_i++;
		}
		cur_scoreType++;
	}
	return os;
}

