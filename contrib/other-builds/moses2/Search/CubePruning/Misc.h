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

namespace NSCubePruning
{
class CubeEdge;

///////////////////////////////////////////
class QueueItem
{
	~QueueItem(); // NOT IMPLEMENTED. Use MemPool
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
	  return !orderer(itemA->hypo, itemB->hypo);
  }
};

///////////////////////////////////////////
class CubeEdge
{
  friend std::ostream& operator<<(std::ostream &, const CubeEdge &);

public:
	typedef std::vector<const Hypothesis*>  Hypotheses;
	typedef std::priority_queue<QueueItem*,
				std::vector<QueueItem*>,
				QueueItemOrderer> Queue;

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
	void Finalize();

  bool SeenPosition(const size_t x, const size_t y) const;
  void SetSeenPosition(const size_t x, const size_t y);

  void CreateFirst(Manager &mgr, Queue &queue);
  void CreateNext(Manager &mgr, const QueueItem *ele, Queue &queue);


protected:
    boost::unordered_set< int > m_seenPosition;

};


}


