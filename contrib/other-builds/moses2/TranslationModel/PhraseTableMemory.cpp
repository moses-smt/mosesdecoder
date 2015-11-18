/*
 * PhraseTableMemory.cpp
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#include <cassert>
#include <boost/foreach.hpp>
#include "PhraseTableMemory.h"
#include "../Phrase.h"
#include "../TargetPhrase.h"
#include "../System.h"
#include "../Scores.h"
#include "../InputPaths.h"
#include "../legacy/InputFileStream.h"

using namespace std;

PhraseTableMemory::Node::Node()
{}

PhraseTableMemory::Node::~Node()
{
}

void PhraseTableMemory::Node::AddRule(PhraseImpl &source, TargetPhrase *target)
{
	AddRule(source, target, 0);
}

PhraseTableMemory::Node &PhraseTableMemory::Node::AddRule(PhraseImpl &source, TargetPhrase *target, size_t pos)
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

TargetPhrases::shared_const_ptr PhraseTableMemory::Node::Find(const Phrase &source, size_t pos) const
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

void PhraseTableMemory::Node::SortAndPrune(size_t tableLimit)
{
  BOOST_FOREACH(Children::value_type &val, m_children) {
	  Node &child = val.second;
	  child.SortAndPrune(tableLimit);
  }

  // prune target phrases in this node
  TargetPhrases *tps = m_targetPhrases.get();
  if (tps) {
	  tps->SortAndPrune(tableLimit);
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
	FactorCollection &vocab = system.vocab;

	MemPool tmpPool;
	vector<string> toks;
	size_t lineNum = 0;
	InputFileStream strme(m_path);
	string line;
	while (getline(strme, line)) {
		if (++lineNum % 100000) {
			cerr << lineNum << " ";
		}
		toks.clear();
		TokenizeMultiCharSeparator(toks, line, "|||");
		assert(toks.size() >= 3);
		//cerr << "line=" << line << endl;

		PhraseImpl *source = PhraseImpl::CreateFromString(tmpPool, vocab, system, toks[0]);
		//cerr << "created soure" << endl;
		TargetPhrase *target = TargetPhrase::CreateFromString(system.systemPool, system, toks[1]);
		//cerr << "created target" << endl;
		target->GetScores().CreateFromString(toks[2], *this, system, true);
		//cerr << "created scores" << endl;

		MemPool tmpPool;
		system.featureFunctions.EvaluateInIsolation(tmpPool, system, *source, *target);
		m_root.AddRule(*source, target);
	}

	m_root.SortAndPrune(m_tableLimit);
}

TargetPhrases::shared_const_ptr PhraseTableMemory::Lookup(const Manager &mgr, InputPath &inputPath) const
{
	const SubPhrase &phrase = inputPath.subPhrase;
	TargetPhrases::shared_const_ptr tps = m_root.Find(phrase);
	return tps;
}

