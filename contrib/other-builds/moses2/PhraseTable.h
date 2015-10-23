/*
 * PhraseTable.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_map.hpp>
#include "TargetPhrases.h"
#include "Word.h"
#include "moses/Util.h"

class Node
{

protected:
	typedef boost::unordered_map<Word, Node, Moses::UnorderedComparer<Word>, Moses::UnorderedComparer<Word> > Children;
	Children m_children;

	TargetPhrases m_targetPhrases;
};

class PhraseTable
{
public:
	PhraseTable();
	virtual ~PhraseTable();
	void Load();

protected:
	Node m_root;
};

