#include "CubePruningData.h"
#include "WordsBitmap.h"
#include "Hypothesis.h"
#include "Util.h"

CubePruningData::CubePruningData()
{
	gridNr = 0;
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
//	IFVERBOSE(1) {
		cout << "Delete Cube Pruning data for this sentence!" << endl;
	//}
	xData.clear();
	yData.clear();
}


