/*
 * PhraseTable.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <string>
#include <boost/unordered_map.hpp>
#include "TargetPhrases.h"
#include "Word.h"
#include "StatelessFeatureFunction.h"
#include "moses/Util.h"

class System;
class InputPaths;

class Node
{
public:
	Node();
	~Node();
	void AddRule(Phrase &source, TargetPhrase *target);
	const TargetPhrases *Find(const PhraseBase &source, size_t pos = 0) const;

protected:
	typedef boost::unordered_map<Word, Node, Moses::UnorderedComparer<Word>, Moses::UnorderedComparer<Word> > Children;
	Children m_children;
	TargetPhrases *m_targetPhrases;

	Node &AddRule(Phrase &source, TargetPhrase *target, size_t pos);

};

////////////////////////////////////////////////////////////////////////
class PhraseTable : public StatelessFeatureFunction
{
public:
	PhraseTable(size_t startInd, const std::string &line);
	virtual ~PhraseTable();

	virtual void SetParameter(const std::string& key, const std::string& value);
	void Load(System &system);
	void Lookups(InputPaths &inputPaths) const;

	void SetPtInd(size_t ind)
	{ m_ptInd = ind; }
	size_t GetPtInd() const
	{ return m_ptInd; }

protected:
	Node m_root;
	std::string m_path;
	size_t m_ptInd;

};

