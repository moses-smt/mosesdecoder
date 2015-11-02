/*
 * PhraseTableMemory.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <cassert>
#include <boost/foreach.hpp>
#include "PhraseTableMemory.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "System.h"
#include "Scores.h"
#include "InputPaths.h"
#include "moses/InputFileStream.h"

using namespace std;

PhraseTableMemory::Node::Node()
{}

PhraseTableMemory::Node::~Node()
{
}

void PhraseTableMemory::Node::AddRule(Phrase &source, TargetPhrase *target)
{
	AddRule(source, target, 0);
}

PhraseTableMemory::Node &PhraseTableMemory::Node::AddRule(Phrase &source, TargetPhrase *target, size_t pos)
{
	if (pos == source.GetSize()) {
		TargetPhrases *tp = m_targetPhrases.get();
		if (tp == NULL) {
			tp = new TargetPhrases();
			m_targetPhrases.reset(tp);
		}

		tp->AddTargetPhrase(*target);
		return *this;
	}
	else {
		const Word &word = source[pos];
		Node &child = m_children[word];
		return child.AddRule(source, target, pos + 1);
	}
}

TargetPhrases::shared_const_ptr PhraseTableMemory::Node::Find(const PhraseBase &source, size_t pos) const
{
	assert(source.GetSize());
	if (pos == source.GetSize()) {
		return m_targetPhrases;
	}
	else {
		const Word &word = source[pos];
		//cerr << "word=" << word << endl;
		Children::const_iterator iter = m_children.find(word);
		if (iter == m_children.end()) {
			return TargetPhrases::shared_const_ptr();
		}
		else {
			const Node &child = iter->second;
			return child.Find(source, pos + 1);
		}
	}
}

////////////////////////////////////////////////////////////////////////

PhraseTableMemory::PhraseTableMemory(size_t startInd, const std::string &line)
:PhraseTable(startInd, line)
{
	ReadParameters();
}

PhraseTableMemory::~PhraseTableMemory() {
	// TODO Auto-generated destructor stub
}

void PhraseTableMemory::Load(System &system)
{
	Moses::FactorCollection &vocab = system.GetVocab();

	MemPool tmpPool;
	vector<string> toks;
	Moses::InputFileStream strme(m_path);
	string line;
	while (getline(strme, line)) {
		toks.clear();
		Moses::TokenizeMultiCharSeparator(toks, line, "|||");
		assert(toks.size() >= 3);
		//cerr << "line=" << line << endl;

		Phrase *source = Phrase::CreateFromString(tmpPool, vocab, toks[0]);
		//cerr << "created soure" << endl;
		TargetPhrase *target = TargetPhrase::CreateFromString(system.GetSystemPool(), system, toks[1]);
		//cerr << "created target" << endl;
		target->GetScores().CreateFromString(toks[2], *this, system, true);
		//cerr << "created scores" << endl;

		MemPool tmpPool;
		system.GetFeatureFunctions().EvaluateInIsolation(tmpPool, system, *source, *target);
		m_root.AddRule(*source, target);
	}
}

TargetPhrases::shared_const_ptr PhraseTableMemory::Lookup(const Manager &mgr, InputPath &inputPath) const
{
	const SubPhrase &phrase = inputPath.GetSubPhrase();
	TargetPhrases::shared_const_ptr tps = m_root.Find(phrase);
	return tps;
}
