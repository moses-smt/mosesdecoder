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

namespace Moses2
{

PhraseTableMemory::Node::Node()
:m_unsortedTPS(NULL)
,m_targetPhrases(NULL)
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
		if (m_unsortedTPS == NULL) {
			m_unsortedTPS = new std::vector<TargetPhrase*>();
			m_source = &source;
		}

		m_unsortedTPS->push_back(target);
		return *this;
	}
	else {
		const Word &word = source[pos];
		Node &child = m_children[word];
		return child.AddRule(source, target, pos + 1);
	}
}

TargetPhrases *PhraseTableMemory::Node::Find(const Phrase &source, size_t pos) const
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
			return NULL;
		}
		else {
			const Node &child = iter->second;
			return child.Find(source, pos + 1);
		}
	}
}

void PhraseTableMemory::Node::SortAndPrune(size_t tableLimit, MemPool &pool, System &system)
{
  BOOST_FOREACH(Children::value_type &val, m_children) {
	  Node &child = val.second;
	  child.SortAndPrune(tableLimit, pool, system);
  }

  // prune target phrases in this node
  if (m_unsortedTPS) {
	  m_targetPhrases = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, m_unsortedTPS->size());

	  for (size_t i = 0; i < m_unsortedTPS->size(); ++i) {
		  TargetPhrase *tp = (*m_unsortedTPS)[i];
		  m_targetPhrases->AddTargetPhrase(*tp);
	  }

	  m_targetPhrases->SortAndPrune(tableLimit);
	  system.featureFunctions.EvaluateAfterTablePruning(system.GetSystemPool(), *m_targetPhrases, *m_source);

	  delete m_unsortedTPS;
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

		system.featureFunctions.EvaluateInIsolation(systemPool, system, *source, *target);
		m_root.AddRule(*source, target);
	}

	m_root.SortAndPrune(m_tableLimit, systemPool, system);
}

TargetPhrases* PhraseTableMemory::Lookup(const Manager &mgr, MemPool &pool, InputPath &inputPath) const
{
	const SubPhrase &phrase = inputPath.subPhrase;
	TargetPhrases *tps = m_root.Find(phrase);
	return tps;
}

}

