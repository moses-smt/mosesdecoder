#include "CubePruningData.h"
#include "WordsBitmap.h"
#include "Hypothesis.h"

CubePruningData::CubePruningData()
{
}

CubePruningData::~CubePruningData()
{
}

void CubePruningData::SaveData(const WordsBitmap &hypoBitmap, const vector< Hypothesis*> &coverageVec, const TranslationOptionList &tol)
{
	xData[hypoBitmap] = coverageVec;
	yData[hypoBitmap] = tol;
}
