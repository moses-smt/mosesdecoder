#ifndef CUBEPRUNINGDATA_H_
#define CUBEPRUNINGDATA_H_

#include "WordsBitmap.h"
#include "Hypothesis.h"
#include "HypothesisStack.h"

class CubePruningData
{
public:
	// key of maps is hypothesis ID
	std::map< size_t, std::vector< Hypothesis*> > xData;
	std::map< size_t, TranslationOptionList > yData;
	
	CubePruningData();
	virtual ~CubePruningData();
	
	void SaveData(Hypothesis *hypo, const vector< Hypothesis*> &coverageVec, TranslationOptionList &tol);
	void DeleteData(Hypothesis *hypo);
};

#endif /*CUBEPRUNINGDATA_H_*/



