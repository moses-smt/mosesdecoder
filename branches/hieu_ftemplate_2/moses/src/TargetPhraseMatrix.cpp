// $Id: DecodeStepTranslation.cpp 1432 2007-07-24 21:48:06Z hieuhoang1972 $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "TargetPhraseMatrix.h"
#include "TargetPhraseCollection.h"
#include "StaticData.h"

using namespace std;

Permutation TargetPhraseMatrix::GetAllPermutations(size_t sourceSize)
{
	Permutation prevPerm;
	if (sourceSize == 1)
	{
		std::vector<size_t> initPermute(1, 1);
		prevPerm.insert(initPermute);
		return prevPerm;
	}
	else
	{ // recursive
		prevPerm = GetAllPermutations(sourceSize - 1);
	}

	// perm that encompass the whole sub-range
	Permutation retPerm;
	std::vector<size_t> largePermute(1, sourceSize);
	retPerm.insert(largePermute);

	Permutation::const_iterator iterPerm;
	for (iterPerm = prevPerm.begin(); iterPerm != prevPerm.end(); ++iterPerm)
	{
		const std::vector<size_t> &oldPerm = *iterPerm;

		std::vector<size_t> appendPerm = oldPerm;
		appendPerm.push_back(1);

		std::vector<size_t> prependPerm = oldPerm;
		prependPerm.insert(prependPerm.begin(), 1);

		std::vector<size_t> increaseFrontPerm = oldPerm;
		increaseFrontPerm[0]++;

		std::vector<size_t> increaseBackPerm = oldPerm;
		increaseBackPerm.back()++;

		retPerm.insert(appendPerm);
		retPerm.insert(prependPerm);
		retPerm.insert(increaseFrontPerm);
		retPerm.insert(increaseBackPerm);
	}

	return retPerm;
}

void TargetPhraseMatrix::CreateConcatenatedPhraseList(ConcatenatedPhraseColl &output
																		, size_t tableLimit
																		, bool adhereTableLimit)
{
	set<vector<const TargetPhraseCollection*> > collSet;
	Permutation permutations = TargetPhraseMatrix::GetAllPermutations(m_sourceSize);
	
	/* don't enumerate every possible permuations, delete permuations 
		which cuts range up into too many subranges */
	size_t maxNumSubRanges = StaticData::Instance().GetMaxNumSubRanges();
	if (maxNumSubRanges > 0 && m_sourceSize > maxNumSubRanges)
	{ 
		Permutation::iterator iterPerm = permutations.begin();
		while (iterPerm != permutations.end())
		{
			const std::vector<size_t> &perm = *iterPerm;
			if (perm.size() > maxNumSubRanges)
			{
				Permutation::iterator iterDelete = iterPerm++;
				permutations.erase(iterDelete);
			}
			else
				++iterPerm;
		}
	}

	assert((*permutations.rbegin()).size() == 1);

	// begin from the end. always the 1 that covers the whole range
	Permutation::reverse_iterator iterPerm;
	for (iterPerm = permutations.rbegin(); iterPerm != permutations.rend(); ++iterPerm)
	{
		// create list of trans opt coll to do cartesian product
		const std::vector<size_t> &perm = *iterPerm;

		if (perm.size() > maxNumSubRanges)
			break; // don't segment more than necessary

		vector<SubRangePhraseColl> permTransOptColl(perm.size());

		bool valid = true;
		size_t startPos = 0;
		for (size_t ind = 0 ; ind < perm.size() ; ++ind)
		{
			size_t endPos = startPos + perm[ind] - 1;
			const TargetPhraseCollection *transOptColl = Get(startPos, endPos);
			if (transOptColl == NULL)
			{ /* no target phrase for this range. 
        Forget about the whole target phrase */
				valid = false;
				break;
			}
			else
      {
        WordsRange sourceRange(startPos, endPos);
				permTransOptColl[ind] = SubRangePhraseColl(sourceRange, transOptColl);
      }

			startPos = endPos + 1;
		} //for (size_t ind

		// do cartesian product
		if (valid)
		{
			DoCartesianProduct(output, permTransOptColl, tableLimit, adhereTableLimit);

			// only do for this amount of sementation
			maxNumSubRanges = perm.size();
		}
	} //for (iterPerm
}

/** used in generation: increases iterators when looping through the exponential number of generation expansions */
inline bool IncrementIterators(vector<TargetPhraseCollection::const_iterator> &iterPhraseColl
                               , const vector<SubRangePhraseColl> &phraseColl
															 , const vector<TargetPhraseCollection::const_iterator> &iterEnd
															 , size_t tableLimit
															 , bool adhereTableLimit)
{
  for (size_t ind = 0 ; ind < phraseColl.size() ; ind++)
  {
    TargetPhraseCollection::const_iterator &iter = iterPhraseColl[ind];
    iter++;
    if (iter != iterEnd[ind])
    { // eg. 4 -> 5
      return false;
    }
    else
    { //  eg 9 -> 10
      iter = phraseColl[ind].GetTargetPhraseCollection()->begin();
    }
  }

	for (size_t ind = 0 ; ind < phraseColl.size() ; ind++)
	{
		TargetPhraseCollection::const_iterator &iter = iterPhraseColl[ind];
    if (iter != phraseColl[ind].GetTargetPhraseCollection()->begin())
			return false; // not yet at end
	}
	return true; // at end, went round again
}

void TargetPhraseMatrix::DoCartesianProduct(ConcatenatedPhraseColl &output
													, const std::vector<SubRangePhraseColl> &permTransOptColl
													, size_t tableLimit
													, bool adhereTableLimit)
{
	// initialise iters
	vector<TargetPhraseCollection::const_iterator> 
			iterTargetPhraseColl(permTransOptColl.size())
			,iterEndColl(permTransOptColl.size());
	for (size_t ind = 0 ; ind < permTransOptColl.size() ; ++ind)
	{
    const TargetPhraseCollection &phraseColl = *permTransOptColl[ind].GetTargetPhraseCollection();
		iterTargetPhraseColl[ind] = phraseColl.begin();

		iterEndColl[ind] = (!adhereTableLimit || tableLimit == 0 || phraseColl.GetSize() < tableLimit) 
											?	phraseColl.end() : phraseColl.begin() + tableLimit;
	}

	// MAIN LOOP. create all combinations of permTransOptColl & add to output
	bool eof = false;
	while (!eof)
	{
		// create concate phrase
		ConcatenatedPhrase concatePhrase(permTransOptColl.size());
		for (size_t ind = 0 ; ind < permTransOptColl.size() ; ++ind)
		{
      const WordsRange &sourceRange = permTransOptColl[ind].GetSourceRange();
			const TargetPhrase *targetPhrase = *iterTargetPhraseColl[ind];
      concatePhrase.Set(ind
                      , sourceRange
                      , targetPhrase);
		}
		output.push_back(concatePhrase);

		eof = IncrementIterators(iterTargetPhraseColl
										, permTransOptColl
										, iterEndColl
										, tableLimit
										, adhereTableLimit);
	}
}

