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

#pragma once

#include <set>
#include <vector>
#include "ConcatenatedPhrase.h"
#include "WordsRange.h"

class TargetPhraseCollection;
class TargetPhrase;

typedef std::set< std::vector<size_t> > Permutation;


// helper class
class SubRangePhraseColl
{
protected:
  WordsRange m_sourceRange;
  const TargetPhraseCollection *m_targetPhraseColl;
public:
  SubRangePhraseColl()
  {}
  SubRangePhraseColl(const WordsRange &sourceRange
                     ,const TargetPhraseCollection *targetPhraseColl)
    :m_sourceRange(sourceRange)
    ,m_targetPhraseColl(targetPhraseColl)
  {}
  const WordsRange &GetSourceRange() const
  { return m_sourceRange; }
  const TargetPhraseCollection *GetTargetPhraseCollection() const
  { return m_targetPhraseColl; }
};

/**	All possible permutations of decode step > 0
	*/
class TargetPhraseMatrix
{
protected:
	size_t m_sourceSize;
	typedef std::vector< std::vector<const TargetPhraseCollection*> > Matrix;
	Matrix m_matrix;
public:
	static Permutation GetAllPermutations(size_t sourceSize);

	TargetPhraseMatrix(size_t sourceSize)
		:m_sourceSize(sourceSize)
		,m_matrix(sourceSize)
	{
		for (size_t pos = 0; pos < sourceSize; ++pos)
			m_matrix[pos].resize(sourceSize - pos);
	}

	const TargetPhraseCollection *Get(size_t startPos, size_t endPos)
	{ return m_matrix[startPos][endPos - startPos]; }

	void Add(size_t startPos, size_t endPos, const TargetPhraseCollection *targetPhraseCollection)
	{
		 m_matrix[startPos][endPos - startPos] = targetPhraseCollection;
	}

	void CreateConcatenatedPhraseList(ConcatenatedPhraseColl &output
									, size_t tableLimit
									, bool adhereTableLimit);
	void DoCartesianProduct(ConcatenatedPhraseColl &output
												, const std::vector<SubRangePhraseColl> &permTransOptColl
												, size_t tableLimit
												, bool adhereTableLimit);

};
