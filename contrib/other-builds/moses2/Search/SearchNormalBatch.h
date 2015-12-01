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
#include "Stacks.h"

class Hypothesis;
class InputPath;
class TargetPhrases;
class TargetPhrase;
class Stacks;

class SearchNormalBatch : public Search
{
public:
	SearchNormalBatch(Manager &mgr);
	virtual ~SearchNormalBatch();

	virtual void Decode();

	const Hypothesis *GetBestHypothesis() const;

protected:
    Stacks m_stacks;
	ObjectPoolContiguous<Hypothesis*> *m_batchForEval;

	void Decode(size_t stackInd);
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

};


