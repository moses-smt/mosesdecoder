#ifndef CUBEPRUNINGDATA_H_
#define CUBEPRUNINGDATA_H_

#include "WordsBitmap.h"
#include "Hypothesis.h"
#include "HypothesisStack.h"

class CubePruningData
{
public:
	std::map<const WordsBitmap, vector< Hypothesis*> > xData;
	std::map<const WordsBitmap, TranslationOptionList > yData;
	typedef set<Hypothesis*, HypothesisScoreOrderer > OrderedHypotheses;
	std::map<size_t, OrderedHypotheses > cand; 
	
	CubePruningData();
	virtual ~CubePruningData();
	
	void SaveData(const WordsBitmap &hypoBitmap, const vector< Hypothesis*> &coverageVec, const TranslationOptionList &tol);
};

#endif /*CUBEPRUNINGDATA_H_*/



