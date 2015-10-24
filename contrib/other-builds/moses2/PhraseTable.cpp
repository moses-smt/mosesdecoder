/*
 * PhraseTable.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <cassert>
#include "PhraseTable.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "StaticData.h"
#include "Scores.h"
#include "InputPaths.h"
#include "moses/InputFileStream.h"

using namespace std;

void Node::AddRule(Phrase &source, TargetPhrase *target)
{
	AddRule(source, target, 0);
}

Node &Node::AddRule(Phrase &source, TargetPhrase *target, size_t pos)
{
	if (pos == source.GetSize()) {
		m_targetPhrases.AddTargetPhrase(*target);
		return *this;
	}
	else {
		const Word &word = source[pos];
		Node &child = m_children[word];
		return child.AddRule(source, target, pos + 1);
	}
}

const TargetPhrases *Node::Find(const PhraseBase &source, size_t pos) const
{
	assert(source.GetSize());
	if (pos == source.GetSize() - 1) {
		return &m_targetPhrases;
	}
	else {
		const Word &word = source[pos];
		Children::const_iterator iter = m_children.find(word);
		if (iter == m_children.end()) {
			return NULL;
		}
		else {
			const Node &child = iter->second;
			return child.Find(source, pos + 1);
		}
	}
}

////////////////////////////////////
PhraseTable::PhraseTable(size_t startInd)
:FeatureFunction(startInd)
{
}

PhraseTable::~PhraseTable() {
	// TODO Auto-generated destructor stub
}

void PhraseTable::Load(StaticData &staticData)
{
	m_path = "/Users/hieu/workspace/experiment/issues/sample-models/phrase-model/phrase-table";

	util::Pool tmpPool;
	vector<string> toks;
	Moses::InputFileStream strme(m_path);
	string line;
	while (getline(strme, line)) {
		toks.clear();
		Moses::TokenizeMultiCharSeparator(toks, line, "|||");
		assert(toks.size() >= 3);

		Phrase *source = Phrase::CreateFromString(tmpPool, toks[0]);
		TargetPhrase *target = TargetPhrase::CreateFromString(staticData.GetPool(), staticData, toks[1]);
		target->GetScores().CreateFromString(toks[2], *this, staticData);
		m_root.AddRule(*source, target);
	}
}

void PhraseTable::Lookups(InputPaths &inputPaths) const
{
	InputPaths::iterator iter;
	for (iter = inputPaths.begin(); iter != inputPaths.end(); ++iter) {
		InputPath &path = *iter;
		const SubPhrase &phrase = path.GetSubPhrase();
		const TargetPhrases *tps = m_root.Find(phrase);
	}
}


