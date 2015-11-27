/*
 * CubePruning.h
 *
 *  Created on: 27 Nov 2015
 *      Author: hieu
 */
#pragma once
#include <boost/unordered_map.hpp>
#include <vector>
#include <queue>
#include "../TypeDef.h"
#include "../legacy/Range.h"

class Manager;
class Hypothesis;
class InputPath;
class TargetPhrases;
class Bitmap;

struct CubeEdge
{
public:
	typedef std::vector<const Hypothesis*>  Hypotheses;
	typedef std::pair<const Bitmap*, Range> HypoCoverage;
	  // bitmap and range of hypos
	typedef boost::unordered_map<HypoCoverage, Hypotheses> HyposForCube;

	CubeEdge(Manager &mgr,
			const Hypotheses &hypos,
			const InputPath &path,
			const TargetPhrases &tps,
			const Bitmap &newBitmap);

	const Hypotheses &hypos;
	const InputPath &path;
	const TargetPhrases &tps;
	const Bitmap &newBitmap;
	SCORE estimatedScore;
};

///////////////////////////////////////////
struct CubeElement
{
	CubeElement(Manager &mgr, const CubeEdge &edge, size_t hypoIndex, size_t tpIndex);

	const CubeEdge &edge;
	size_t hypoIndex, tpIndex;
	Hypothesis *hypo;

	void CreateHypothesis(Manager &mgr);
};
