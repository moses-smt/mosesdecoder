// $Id$

#include <iostream>
#include <limits>
#include <cassert>
#include "DistortionOrientation.h"
#include "TypeDef.h"
#include "Hypothesis.h"
using namespace std;
/*
 * Load the file pointed to by filename; set up the table according to
 * the orientation and condition parameters. Direction will be used
 * later for computing the score.
 * 
 * default type is Msd, meaning will distinguish between monotone, swap, discontinuous rather than
 * just monotone/non-monotone.
 */
int DistortionOrientation::GetOrientation(const Hypothesis *curr_hypothesis, int direction, int type) 
{
	const Hypothesis *prevHypo = curr_hypothesis->GetPrevHypo();
	if(prevHypo==NULL){
		//if there's no previous source we judge the first hypothesis as monotone.
		return DistortionOrientationType::MONO;		
	}
	const WordsRange &currTargetRange = curr_hypothesis->GetCurrTargetWordsRange()
			       , &currSourceRange = curr_hypothesis->GetCurrSourceWordsRange();
	size_t curr_source_start = currSourceRange.GetStartPos();
	size_t curr_source_end = currSourceRange.GetEndPos();
	const WordsRange &prevSourceRange = prevHypo->GetCurrSourceWordsRange();
	size_t  prev_source_start = prevSourceRange.GetStartPos();
	size_t prev_source_end = prevSourceRange.GetEndPos();		
	if(prev_source_end==curr_source_start-1)
	{
		return DistortionOrientationType::MONO;
	}
	else if(type==DistortionOrientationType::Msd) //distinguish between monotone, swap, discontinuous
	{
		if(prev_source_start==curr_source_end+1)
		{
			return DistortionOrientationType::SWAP;
		}
		else
		{
			return DistortionOrientationType::DISC;
		}
	}
	else //only distinguish between Monotone, non monotone
	{
		return DistortionOrientationType::NON_MONO;
	}
}

