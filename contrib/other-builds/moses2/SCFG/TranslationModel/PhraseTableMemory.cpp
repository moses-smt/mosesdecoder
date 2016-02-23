/*
 * PhraseTableMemory.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include "PhraseTableMemory.h"
#include "../../legacy/FactorCollection.h"
#include "../../legacy/InputFileStream.h"
#include "../../System.h"
#include "../../MemPool.h"
#include "../../Scores.h"
#include "../TargetPhrase.h"

using namespace std;

namespace Moses2
{
namespace SCFG
{

void PhraseTableMemory::Load(System &system)
{
	FactorCollection &vocab = system.GetVocab();

	MemPool &systemPool = system.GetSystemPool();
	MemPool tmpSourcePool;
	vector<string> toks;
	size_t lineNum = 0;
	InputFileStream strme(m_path);
	string line;
	while (getline(strme, line)) {
		if (++lineNum % 1000000 == 0) {
			cerr << lineNum << " ";
		}
		toks.clear();
		TokenizeMultiCharSeparator(toks, line, "|||");
		assert(toks.size() >= 3);
		//cerr << "line=" << line << endl;

		PhraseImpl *source = PhraseImpl::CreateFromString(tmpSourcePool, vocab, system, toks[0]);
		//cerr << "created soure" << endl;
		TargetPhrase *target = TargetPhrase::CreateFromString(systemPool, *this, system, toks[1]);
		//cerr << "created target" << endl;
		target->GetScores().CreateFromString(toks[2], *this, system, true);
		//cerr << "created scores" << endl;

		// properties
		if (toks.size() == 7) {
			//target->properties = (char*) system.systemPool.Allocate(toks[6].size() + 1);
			//strcpy(target->properties, toks[6].c_str());
		}

		//system.featureFunctions.EvaluateInIsolation(systemPool, system, *source, *target);
		//m_root.AddRule(*source, target);
	}
	m_root.SortAndPrune(m_tableLimit, systemPool, system);
}

}
}

