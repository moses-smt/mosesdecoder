/*
 * SearchNormalBatch.h
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include "../legacy/Range.h"
#include "../legacy/Bitmap.h"
#include "../TypeDef.h"
#include "../MemPool.h"
#include "../Recycler.h"
#include "ArcLists.h"
#include "Search.h"

class Hypothesis;
class InputPath;
class TargetPhrases;
class TargetPhrase;
class Stacks;

class SearchNormalBatch : public Search
{
public:
	SearchNormalBatch(Manager &mgr, Stacks &stacks);
	virtual ~SearchNormalBatch();

	void Decode(size_t stackInd);

	const Hypothesis *GetBestHypothesis() const;

protected:
	Recycler<Hypothesis*> *m_batchForEval;

	void Extend(const Hypothesis &hypo);
	void Extend(const Hypothesis &hypo, const InputPath &path);
	void Extend(const Hypothesis &hypo,
			const TargetPhrases &tps,
			const Range &pathRange,
			const Bitmap &newBitmap,
			SCORE estimatedScore);
	void Extend(const Hypothesis &hypo,
			const TargetPhrase &tp,
			const Range &pathRange,
			const Bitmap &newBitmap,
			SCORE estimatedScore);

	void AddHypos();

	int ComputeDistortionDistance(const Range& prev, const Range& current) const;

};

