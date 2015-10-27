/*
 * FeatureFunction.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <string>
#include <vector>
#include "FeatureFunction.h"
#include "System.h"
#include "moses/Util.h"

#include "PhraseTable.h"

using namespace std;

FeatureFunction::FeatureFunction(size_t startInd, const std::string &line)
:m_startInd(startInd)
,m_numScores(1)
{
}

FeatureFunction::~FeatureFunction() {
	// TODO Auto-generated destructor stub
}

FeatureFunction *FeatureFunction::Create(System &system, const std::string &line)
{
	vector<string> toks = Moses::Tokenize(line);

	FeatureFunction *ret;
	if (toks[0] == "PhraseDictionaryMemory") {
		ret = new PhraseTable(system.GetFFStartInd(), line);
	}
}
