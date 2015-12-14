/*
 * PhraseTableMemory.h
 *
 *  Created on: 28 Oct 2015
 *      Author: hieu
 */

#ifndef PHRASETABLEMEMORY_H_
#define PHRASETABLEMEMORY_H_
#include "PhraseTable.h"
#include "../legacy/Util2.h"

class PhraseTableMemory : public PhraseTable
{
//////////////////////////////////////

  /**
   * Implements a linked list of source words in a hash table.
   */
	class Node
	{
	public:
		Node();
		~Node();
		void AddRule(PhraseImpl &source, TargetPhrase *target);
		TargetPhrases::shared_const_ptr Find(const Phrase &source, size_t pos = 0) const;

		void SortAndPrune(size_t tableLimit);

	protected:
		typedef boost::unordered_map<Word, Node, UnorderedComparer<Word>, UnorderedComparer<Word> > Children;
		Children m_children;
		TargetPhrases::shared_ptr m_targetPhrases;

		Node &AddRule(PhraseImpl &source, TargetPhrase *target, size_t pos);

	};
//////////////////////////////////////
public:
	PhraseTableMemory(size_t startInd, const std::string &line);
	virtual ~PhraseTableMemory();

	virtual void Load(System &system);
	virtual TargetPhrases::shared_const_ptr Lookup(const Manager &mgr, InputPath &inputPath) const;

protected:
	Node m_root;
};

#endif /* PHRASETABLEMEMORY_H_ */
