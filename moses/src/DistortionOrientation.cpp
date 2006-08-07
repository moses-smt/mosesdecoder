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
 * just monotone/non monotone.
 */
int DistortionOrientation::GetOrientation(const Hypothesis *curr_hypothesis, int direction, int type) 
{
	size_t numSourceWords = curr_hypothesis->GetWordsBitmap().GetSize();
	const WordsRange &currTargetRange = curr_hypothesis->GetCurrTargetWordsRange()
			       , &currSourceRange = curr_hypothesis->GetCurrSourceWordsRange();
	const Hypothesis *prevHypo = curr_hypothesis->GetPrevHypo();
	size_t curr_source_start = currSourceRange.GetStartPos();
	size_t curr_source_end = currSourceRange.GetEndPos();
	size_t curr_target_end = currTargetRange.GetEndPos();
	size_t prev_source_start = NULL;
	size_t prev_source_end = NULL;
	if(prevHypo!=NULL){
		//don't look for attributes of the previous hypothesis if there is no previous hypothesis.
		const WordsRange &prevSourceRange = prevHypo->GetCurrSourceWordsRange();
		prev_source_start = prevSourceRange.GetStartPos();
		prev_source_end = prevSourceRange.GetEndPos();		
	}
	else{
		return DistortionOrientationType::MONO;		
	}
	if((curr_target_end==numSourceWords && type==LexReorderType::Forward) || prev_source_end==curr_source_start-1)
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

