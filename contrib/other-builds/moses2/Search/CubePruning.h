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
#include "../TypeDef.h"
#include "../legacy/Range.h"

class Manager;
class Hypothesis;
class InputPath;
class TargetPhrases;
class Bitmap;
class CubeElement;

struct CubeEdge
{
  friend std::ostream& operator<<(std::ostream &, const CubeEdge &);

public:
	typedef std::vector<const Hypothesis*>  Hypotheses;
	typedef std::pair<const Bitmap*, Range> HypoCoverage;
	  // bitmap and range of hypos
	typedef boost::unordered_map<HypoCoverage, Hypotheses> HyposForCube;
	typedef std::queue<CubeElement*> Queue;

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

  void CreateNext(Manager &mgr, const CubeElement &ele, Queue &queue);


protected:
    boost::unordered_set< int > m_seenPosition;

};

///////////////////////////////////////////
struct CubeElement
{
	CubeElement(Manager &mgr, CubeEdge &edge, size_t hypoIndex, size_t tpIndex);

	CubeEdge &edge;
	size_t hypoIndex, tpIndex;
	Hypothesis *hypo;

	void CreateHypothesis(Manager &mgr);
};
