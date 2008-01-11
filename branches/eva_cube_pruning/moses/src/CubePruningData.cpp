#include "CubePruningData.h"
#include "WordsBitmap.h"
#include "Hypothesis.h"

CubePruningData::CubePruningData()
{
}

CubePruningData::~CubePruningData()
{
}

void CubePruningData::SaveData(Hypothesis *hypo, const vector< Hypothesis*> &coverageVec, TranslationOptionList &tol)
{
	xData[hypo->GetId()] = coverageVec;
	yData[hypo->GetId()] = tol;
}

void CubePruningData::DeleteData()
{
	cout << "Delete Cube Pruning data for this sentence! " << endl;
	xData.clear();
	yData.clear();
}


