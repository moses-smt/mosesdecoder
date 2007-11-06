#ifndef SCOREGRID_H_
#define SCOREGRID_H_

#endif /*SCOREGRID_H_*/

#include "TypeDef.h"
#include "Hypothesis.h"

class ScoreGrid
{
	protected:	
	
		int m_k;  // size of the two dimensions of the grid
		vector< vector<float> > m_scoreGrid;
		
	public:
		
		ScoreGrid(int k);
		~ScoreGrid();
		
		void FillGrid(std::vector<Hypothesis*> hypos, std::vector<float> scores);
		void PrintGrid();
		
};





