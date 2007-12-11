#include "CubePruningData.h"
#include "WordsBitmap.h"
#include "Hypothesis.h"

CubePruningData::CubePruningData()
{
}

CubePruningData::~CubePruningData()
{
}

//void CubePruningData::SaveData(const WordsBitmap &hypoBitmap, const vector< Hypothesis*> &coverageVec, const TranslationOptionList &tol)
void CubePruningData::SaveData(Hypothesis *hypo, const vector< Hypothesis*> &coverageVec, TranslationOptionList &tol)
{
	xData[hypo->GetId()] = coverageVec;
	yData[hypo->GetId()] = tol;
}

void CubePruningData::DeleteData(Hypothesis *hypo)
{
	xData.erase(hypo->GetId());
	yData.erase(hypo->GetId());
}
