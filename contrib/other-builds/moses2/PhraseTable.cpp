/*
 * PhraseTable.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#include "PhraseTable.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "StaticData.h"
#include "Scores.h"
#include "moses/InputFileStream.h"

using namespace std;

PhraseTable::PhraseTable()
{
	m_startInd = 0;
}

PhraseTable::~PhraseTable() {
	// TODO Auto-generated destructor stub
}

void PhraseTable::Load(StaticData &staticData)
{
	m_path = "/Users/hieu/workspace/experiment/issues/sample-models/phrase-model/phrase-table";

	vector<string> toks;
	Moses::InputFileStream strme(m_path);
	string line;
	while (getline(strme, line)) {
		toks.clear();
		Moses::TokenizeMultiCharSeparator(toks, line, "|||");
		assert(toks.size() >= 3);

		Phrase *source = Phrase::CreateFromString(staticData.GetPool(), toks[0]);
		TargetPhrase *target = TargetPhrase::CreateFromString(staticData.GetPool(), staticData, toks[1]);
		target->GetScores().CreateFromString(toks[2], *this, staticData);
	}
}
