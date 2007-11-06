#include "ScoreGrid.h"
#include "TypeDef.h"
#include "Hypothesis.h"



ScoreGrid::ScoreGrid(int k) : m_k(k)
{
	// fill vector with m_k*m_k elements 0.0
	std:vector<float> dummy;
	for(size_t i=0; i<m_k; i++)
		dummy.push_back(0.0);
	for(size_t i=0; i<m_k; i++)
		m_scoreGrid.push_back(dummy);
}

ScoreGrid::~ScoreGrid()
{
}

void ScoreGrid::FillGrid(std::vector<Hypothesis*> hypos, std::vector<float> scores)
{
	std::vector<Hypothesis*>::iterator hypo_iter;
	hypo_iter = hypos.begin();
	std::vector<float>::iterator score_iter;
	
	for(size_t i=0; i < m_k; i++)
	{	
		score_iter = scores.begin();
		for(size_t j=0; j< m_k; j++)
		{
			if( (hypo_iter != hypos.end()) && (score_iter != scores.end()) )
			{
				m_scoreGrid[i][j] = (*hypo_iter)->GetTotalScore() + *score_iter;
			}
			score_iter++;
		}
		hypo_iter++;
	}
	std::cout << endl;
}

void ScoreGrid::PrintGrid()
{
	for(size_t i=0; i< m_k; i++)
		for(size_t j=0; j< m_k; j++)
			std::cout << i+1 << " " << j+1 << " --> " << m_scoreGrid[i][j] << std::endl;
	std::cout << std::endl;
}







