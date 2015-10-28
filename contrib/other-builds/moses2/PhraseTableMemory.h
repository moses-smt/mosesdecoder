/*
 * PhraseTableMemory.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef PHRASETABLEMEMORY_H_
#define PHRASETABLEMEMORY_H_
#include "PhraseTable.h"

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


class PhraseTableMemory : public PhraseTable
{
public:
	PhraseTableMemory(size_t startInd, const std::string &line);
	virtual ~PhraseTableMemory();

	virtual void Load(System &system);
	virtual TargetPhrases::shared_const_ptr Lookup(const Manager &mgr, InputPath &inputPath) const;

protected:
	Node m_root;
};

#endif /* PHRASETABLEMEMORY_H_ */
