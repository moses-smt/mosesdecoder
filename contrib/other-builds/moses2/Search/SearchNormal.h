/*
 * SearchNormal.h
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#ifndef SEARCHNORMAL_H_
#define SEARCHNORMAL_H_
#include <vector>
#include "../legacy/Range.h"
#include "../legacy/Bitmap.h"
#include "../TypeDef.h"
#include "ArcLists.h"
#include "Search.h"
#include "Stacks.h"

class Hypothesis;
class InputPath;
class TargetPhrases;
class TargetPhrase;
class Stacks;

class SearchNormal : public Search
{
public:
	SearchNormal(Manager &mgr);
	virtual ~SearchNormal();

	virtual void Decode();
	const Hypothesis *GetBestHypothesis() const;

protected:
    Stacks m_stacks;

	void Decode(Stack* stack);
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

};

#endif /* SEARCHNORMAL_H_ */
