/*
 * Stack.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include "../Hypothesis.h"
#include "Misc.h"
#include "../../Recycler.h"
#include "../../TypeDef.h"
#include "../../Vector.h"
#include "../../MemPool.h"
#include "../../legacy/Util2.h"

namespace Moses2
{

class Manager;

namespace NSCubePruning
{

class MiniStack
{
public:
	typedef boost::unordered_set<const Hypothesis*,
			  UnorderedComparer<Hypothesis>,
			  UnorderedComparer<Hypothesis>
			   > _HCType;

	MiniStack()
	{}

	_HCType &GetColl()
	{ return m_coll; }

	const _HCType &GetColl() const
	{ return m_coll; }

	CubeEdge::Hypotheses &GetSortedAndPruneHypos(const Manager &mgr) const;

protected:
	_HCType m_coll;
	mutable CubeEdge::Hypotheses *m_sortedHypos;

	void SortAndPruneHypos(const Manager &mgr) const;

};

/////////////////////////////////////////////
class Stack {
protected:


public:
  typedef std::pair<const Bitmap*, size_t> HypoCoverage;
		  // bitmap and current endPos of hypos

  typedef boost::unordered_map<HypoCoverage,
		  MiniStack,
		  boost::hash<HypoCoverage>,
		  std::equal_to<HypoCoverage>,
		  MemPoolAllocator< std::pair<HypoCoverage const, MiniStack> >
		  > Coll;


	Stack(const Manager &mgr);
	virtual ~Stack();

	size_t GetHypoSize() const;

	Coll &GetColl()
	{ return m_coll; }

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

	std::vector<const Hypothesis*> GetBestHypos(size_t num) const;
	void Clear()
	{
		m_coll.clear();
	}

protected:
	Coll m_coll;

	StackAdd Add(const Hypothesis *hypo);

	MiniStack &GetMiniStack(const HypoCoverage &key);

};

}

}


