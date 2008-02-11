#ifndef CUBEPRUNINGDATA_H_
#define CUBEPRUNINGDATA_H_

#include "WordsBitmap.h"
#include "WordsRange.h"
#include "Hypothesis.h"
#include "HypothesisStack.h"
#include "TranslationOption.h"

class CubePruningData
{
public:

	std::map< WordsBitmap, std::vector< Hypothesis*> > covVecs;
	std::map< WordsRange, TranslationOptionList > tols;
	
	CubePruningData();
	~CubePruningData();
	
	void SaveCoverageVector(const WordsBitmap &wb, const vector< Hypothesis*> &coverageVec);
	void SaveTol(const WordsRange wr, const TranslationOptionList &tol);
	
	bool EntryExists(const WordsRange wr){	return ( tols.find(wr) != tols.end() ); }
	
	void DeleteData();
	
};

#endif /*CUBEPRUNINGDATA_H_*/



