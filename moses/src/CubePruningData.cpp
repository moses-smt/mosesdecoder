#include "CubePruningData.h"
#include "WordsBitmap.h"
#include "Hypothesis.h"
#include "Util.h"
#include "TranslationOption.h"

CubePruningData::CubePruningData()
{
}

CubePruningData::~CubePruningData()
{
}


void CubePruningData::SaveCoverageVector(const WordsBitmap &wb, const vector< Hypothesis*> &coverageVec)
{
	covVecs[wb] = coverageVec;
}


void CubePruningData::SaveTol(const WordsRange wr, const TranslationOptionList &tol)
{
	tols[wr] = tol;
}


void CubePruningData::DeleteData()
{
	cout << "Delete Cube Pruning data for this sentence!" << endl;
	covVecs.clear();
	tols.clear();
}


