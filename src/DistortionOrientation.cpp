// $Id$

#include <iostream>
#include <limits>
#include <assert.h>
#include "DistortionOrientation.h"
#include "TypeDef.h"
#include "Hypothesis.h"
/*
 * Load the file pointed to by filename; set up the table according to
 * the orientation and condition parameters. Direction will be used
 * later for computing the score.
 * 
 * default type is Msd, meaning will distinguish between monotone, swap, discontinuous rather than
 * just monotone/non monotone.
 */
int DistortionOrientation::GetOrientation(const Hypothesis *curr_hypothesis, int direction, int type) 
{
	size_t numSourceWords = curr_hypothesis->GetWordsBitmap().GetSize();
	const WordsRange &currTargetRange = curr_hypothesis->GetPrevHypo()->GetCurrSourceWordsRange()
			   , &prevSourceRange = curr_hypothesis->GetCurrSourceWordsRange()
			   , &currSourceRange = curr_hypothesis->GetCurrTargetWordsRange();

	size_t prev_source_start = prevSourceRange.GetStartPos();
	size_t prev_source_end = prevSourceRange.GetEndPos();
	size_t curr_source_start = currSourceRange.GetStartPos();
	size_t curr_source_end = currSourceRange.GetEndPos();
	size_t curr_target_start = currTargetRange.GetStartPos();
	size_t curr_target_end = currTargetRange.GetEndPos();

	if(direction==LexReorderType::Backward)
	{
		if(curr_target_start==0 || curr_target_end==numSourceWords || prev_source_end==curr_source_start)
		{
			//the first two conditionals are edge cases which judge first and last phrases as monotonic
			return DistortionOrientationType::MONO;
		}
		else if(type==DistortionOrientationType::Msd) //distinguish between monotone, swap, discontinuous
		{
			if(prev_source_start==curr_source_end)
				return DistortionOrientationType::SWAP;
			else
				return DistortionOrientationType::DISC;
		}
		else //only distinguish between Monotone, non monotone
		{
			return DistortionOrientationType::NON_MONO;
		}
		
	}
	else //assume direction is forward, do same computation but on PREVIOUS hypothesis
	{
		return DistortionOrientation::GetOrientation(curr_hypothesis->GetPrevHypo()
													 ,LexReorderType::Forward
													 ,type);
	}
}

