/*
 * CubePruning.h
 *
 *  Created on: 27 Nov 2015
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <vector>
#include <queue>
#include "../Hypothesis.h"
#include "../../TypeDef.h"
#include "../../legacy/Range.h"

class Manager;
class InputPath;
class TargetPhrases;
class Bitmap;
class CubeEdge;

///////////////////////////////////////////
class QueueItem
{
public:
	QueueItem(Manager &mgr, CubeEdge &edge, size_t hypoIndex, size_t tpIndex);

	CubeEdge &edge;
	size_t hypoIndex, tpIndex;
	Hypothesis *hypo;

protected:
	void CreateHypothesis(Manager &mgr);
};

///////////////////////////////////////////
class QueueItemOrderer
{
public:
  bool operator()(QueueItem* itemA, QueueItem* itemB) const {
	  HypothesisFutureScoreOrderer orderer;
	  return orderer(itemA->hypo, itemB->hypo);
  }
};

///////////////////////////////////////////
class CubeEdge
{
  friend std::ostream& operator<<(std::ostream &, const CubeEdge &);

public:
	typedef std::vector<const Hypothesis*>  Hypotheses;
	typedef std::priority_queue<QueueItem*, std::vector< QueueItem* >, QueueItemOrderer> Queue;

	const Hypotheses &hypos;
	const InputPath &path;
	const TargetPhrases &tps;
	const Bitmap &newBitmap;
	SCORE estimatedScore;

	CubeEdge(Manager &mgr,
			const Hypotheses &hypos,
			const InputPath &path,
			const TargetPhrases &tps,
			const Bitmap &newBitmap);

  bool SeenPosition(const size_t x, const size_t y) const;
  void SetSeenPosition(const size_t x, const size_t y);

  void CreateFirst(Manager &mgr, Queue &queue);
  void CreateNext(Manager &mgr, const QueueItem &ele, Queue &queue);


protected:
    boost::unordered_set< int > m_seenPosition;

};

///////////////////////////////////////////
class HyposForCubePruning
{
public:
  typedef std::pair<const Bitmap*, size_t> HypoCoverage;
	  // bitmap and current endPos of hypos
  typedef boost::unordered_map<HypoCoverage, CubeEdge::Hypotheses> Coll;

  typedef Coll::value_type value_type;
  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;

  //! iterators
  const_iterator begin() const {
	return m_coll.begin();
  }
  const_iterator end() const {
	return m_coll.end();
  }
  iterator begin() {
	return m_coll.begin();
  }
  iterator end() {
	return m_coll.end();
  }

	CubeEdge::Hypotheses &GetOrCreate(const Bitmap &bitmap, size_t endPos);

protected:
	Coll m_coll;
};


