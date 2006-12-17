// $Id$

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdio>
#include <cassert>
#include "Util.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"
#include "ScoreProducer.h"
#include "ScoreComponentCollection.h" // debugging

void ScoreIndexManager::AddScoreProducer(ScoreProducer* sp)
{
	// Producers must be inserted in the order they are created
	sp->CreateScoreBookkeepingID();
  assert(m_begins.size() == (sp->GetScoreBookkeepingID()));
  
	m_producers.push_back(sp);
  m_begins.push_back(m_last);
	size_t numScoreCompsProduced = sp->GetNumScoreComponents();
	assert(numScoreCompsProduced > 0);
	m_last += numScoreCompsProduced;
	m_ends.push_back(m_last);
	VERBOSE(2,"Added ScoreProducer(" << sp << "): id=" << sp->GetScoreBookkeepingID() << std::endl);
}

void ScoreIndexManager::Debug_PrintLabeledScores(std::ostream& os, const ScoreComponentCollection& scc) const
{
	std::vector<float> weights(scc.m_scores.size(), 1.0f);
	Debug_PrintLabeledWeightedScores(os, scc, weights);
}

static std::string getFormat(float foo) 
{
  char buf[32];
  sprintf(buf, "%.4f", foo);
  return buf;
}

void ScoreIndexManager::Debug_PrintLabeledWeightedScores(std::ostream& os, const ScoreComponentCollection& scc, const std::vector<float>& weights) const
{
  size_t cur_i = 0;
  size_t cur_scoreType = 0;
  while (cur_i < m_last) {
    bool first = true;
		
		size_t nis_idx = 0;
		while (nis_idx < m_producers[cur_scoreType]->GetNumInputScores()){
      os << "  " << getFormat(scc.m_scores[cur_i]) << "\t" << getFormat(scc.m_scores[cur_i] * weights[cur_i]) << "\t  " << (cur_i < 10 ? " " : "") << cur_i << " ";
			if (first) {
				os << m_producers[cur_scoreType]->GetScoreProducerDescription(1)
				<< std::endl;
				first = false;
			} else {
				os << "    \"         \"" << std::endl;
			}
			nis_idx++;
			cur_i++;
		}
		
		first = true;
    while (cur_i < m_ends[cur_scoreType]) {
      os << "  " << getFormat(scc.m_scores[cur_i]) << "\t" << getFormat(scc.m_scores[cur_i] * weights[cur_i]) << "\t  " << (cur_i < 10 ? " " : "") << cur_i << " ";
      if (first) {
        os << m_producers[cur_scoreType]->GetScoreProducerDescription()
				   << std::endl;
        first = false;
      } else {
        os << "    \"         \"" << std::endl;
      }
      cur_i++;
    }
    cur_scoreType++;
  }
}

std::ostream& operator<<(std::ostream& os, const ScoreIndexManager& sim)
{
	size_t cur_i = 0;
	size_t cur_scoreType = 0;
	while (cur_i < sim.m_last) {
		bool first = true;

		size_t nis_idx = 0;
		while (nis_idx < sim.m_producers[cur_scoreType]->GetNumInputScores()){
			os << "  " << (cur_i < 10 ? " " : "") << cur_i << " ";
			if (first) {
				os << sim.m_producers[cur_scoreType]->GetScoreProducerDescription(1)
				   << std::endl;
				first = false;
			} else {
				os << "    \"         \"" << std::endl;
			}
			nis_idx++;
			cur_i++;
		}

		first = true;
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

