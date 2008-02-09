#include "CubePruningData.h"
#include "WordsBitmap.h"
#include "WordsRange.h"
#include "Hypothesis.h"
#include "Util.h"


CubePruningData::CubePruningData()
{
}

CubePruningData::~CubePruningData()
{
}

/*
void CubePruningData::SaveData(Hypothesis *hypo, const vector< Hypothesis*> &coverageVec, TranslationOptionList &tol)
{
	xData[hypo->GetId()] = coverageVec;
	yData[hypo->GetId()] = tol;
}*/

void CubePruningData::SaveSourceHypoColl(const WordsBitmap &wb, const vector< Hypothesis*> &coverageVec)
{
	xData[wb] = coverageVec;
}

void CubePruningData::SaveTol(const WordsRange &wr, TranslationOptionList &tol)
{
	yData[wr] = tol;
}

void CubePruningData::DeleteData()
{
//	IFVERBOSE(1) {
		cout << "Delete Cube Pruning data for this sentence!" << endl;
	//}
	xData.clear();
	yData.clear();
}


