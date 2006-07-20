// $Id$

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

#include "assert.h"
#include <algorithm>
#include <sstream>
#include "memory.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "Util.h"

using namespace std;

Phrase::Phrase(const Phrase &copy)
:m_direction(copy.m_direction)
,m_phraseSize(copy.m_phraseSize)
,m_arraySize(copy.m_arraySize)
{
	if (m_phraseSize==0)
	{
		m_factorArray = NULL;
	}
	else
	{
		m_factorArray = (FactorArray*) malloc(m_arraySize * sizeof(FactorArray));
		memcpy(m_factorArray, copy.m_factorArray, m_phraseSize * sizeof(FactorArray));
	}
}

Phrase::Phrase(FactorDirection direction, const vector< const Word* > &mergeWords)
:m_direction(direction)
,m_phraseSize(mergeWords.size())
,m_arraySize(mergeWords.size())
{
	m_factorArray = (FactorArray*) malloc(m_arraySize * sizeof(FactorArray));
	
	for (size_t currPos = 0 ; currPos < m_phraseSize ; currPos++)
	{
		FactorArray &thisWord				= m_factorArray[currPos];
		const Word &mergeWord				= *mergeWords[currPos];

		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			thisWord[currFactor] = mergeWord.GetFactor(factorType);
		}
	}
}

Phrase::~Phrase()
{
	free (m_factorArray);
}

void Phrase::MergeFactors(const Phrase &copy)
{
	assert(GetSize() == copy.GetSize());
	size_t size = GetSize();
	for (size_t currPos = 0 ; currPos < size ; currPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *factor = copy.GetFactor(currPos, factorType);
			if (factor != NULL)
				SetFactor(currPos, factorType, factor);
		}
	}
}

void Phrase::AddWords(const Phrase &copy)
{
	for (size_t pos = 0 ; pos < copy.GetSize() ; pos++)
	{
		FactorArray &newWord = AddWord();
		Word::Copy(newWord, copy.GetFactorArray(pos));
	}
}

Phrase Phrase::GetSubString(const WordsRange &wordsRange) const
{
	Phrase retPhrase(m_direction);

	for (size_t currPos = wordsRange.GetStartPos() ; currPos <= wordsRange.GetEndPos() ; currPos++)
	{
		FactorArray &newWord = retPhrase.AddWord();
		Word::Copy(newWord, GetFactorArray(currPos));
	}

	return retPhrase;
}

FactorArray &Phrase::AddWord()
{
	if (m_phraseSize % ARRAY_SIZE_INCR == 0)
	{ // need to expand array
		m_arraySize += ARRAY_SIZE_INCR;
		m_factorArray = (FactorArray*) realloc(m_factorArray, m_arraySize * sizeof(FactorArray));			
	}

	FactorArray &factorArray = m_factorArray[m_phraseSize];
	Word::Initialize(factorArray);

	m_phraseSize++;

	return factorArray;
}

vector< vector<string> > Phrase::Parse(const std::string &phraseString)
{
		// parse
	vector< vector<string> > phraseVector;
	vector<string> annotatedWordVector = Tokenize(phraseString);
	// KOMMA|none ART|Def.Z NN|Neut.NotGen.Sg VVFIN|none 
	//		to
	// "KOMMA|none" "ART|Def.Z" "NN|Neut.NotGen.Sg" "VVFIN|none"

	for (size_t phrasePos = 0 ; phrasePos < annotatedWordVector.size() ; phrasePos++)
	{
		string &annotatedWord = annotatedWordVector[phrasePos];
		vector<string> factorStrVector = Tokenize(annotatedWord, "|");
		// KOMMA|none
		//    to
		// "KOMMA" "none"
		phraseVector.push_back(factorStrVector);
	}
	return phraseVector;
}

void Phrase::CreateFromString(const std::vector<FactorType> &factorOrder
															, const vector< vector<string> > &phraseVector
															, FactorCollection &factorCollection)
{
	for (size_t phrasePos = 0 ; phrasePos < phraseVector.size() ; phrasePos++)
	{
		// add word this phrase
		FactorArray &factorArray = AddWord();
		for (size_t currFactorIndex= 0 ; currFactorIndex < factorOrder.size() ; currFactorIndex++)
		{
			FactorType factorType = factorOrder[currFactorIndex];
			const string &factorStr = phraseVector[phrasePos][currFactorIndex];
			const Factor *factor = factorCollection.AddFactor(m_direction, factorType, factorStr); 
			factorArray[factorType] = factor;
		}
	}
}

void Phrase::CreateFromString(const std::vector<FactorType> &factorOrder
															, const string &phraseString
															, FactorCollection &factorCollection)
{
	vector< vector<string> > phraseVector = Parse(phraseString);
	CreateFromString(factorOrder, phraseVector, factorCollection);
}

bool Phrase::operator< (const Phrase &compare) const
{	
#ifdef min
#undef min
#endif
	size_t thisSize			= GetSize()
				,compareSize	= compare.GetSize();

	// decide by using length. quick decision
	if (thisSize != compareSize)
	{
		return thisSize < compareSize;
	}
	else
	{
		size_t minSize = std::min( thisSize , compareSize );

		// taken from word.Compare()
		for (size_t i = 0 ; i < NUM_FACTORS ; i++)
		{
			FactorType factorType = static_cast<FactorType>(i);

			for (size_t currPos = 0 ; currPos < minSize ; currPos++)
			{
				const Factor *thisFactor		= GetFactor(currPos, factorType)
										,*compareFactor	= compare.GetFactor(currPos, factorType);

				if (thisFactor != NULL && compareFactor != NULL)
				{
					const int result = thisFactor->Compare(*compareFactor);
					if (result == 0)
					{
						continue;
					}
					else 
					{
						return (result < 0);
					}
				}
			}
		}

		// identical
		return false;
	}
}

bool Phrase::Contains(const vector< vector<string> > &subPhraseVector
										, const vector<FactorType> &inputFactor) const
{
	const size_t subSize = subPhraseVector.size()
							,thisSize= GetSize();
	if (subSize > thisSize)
		return false;

	// try to match word-for-word
	for (size_t currStartPos = 0 ; currStartPos < (thisSize - subSize + 1) ; currStartPos++)
	{
		bool match = true;

		for (size_t currFactorIndex = 0 ; currFactorIndex < inputFactor.size() ; currFactorIndex++)
		{
			FactorType factorType = inputFactor[currFactorIndex];
			for (size_t currSubPos = 0 ; currSubPos < subSize ; currSubPos++)
			{
				size_t currThisPos = currSubPos + currStartPos;
				const string &subStr	= subPhraseVector[currSubPos][currFactorIndex]
										,&thisStr	= GetFactor(currThisPos, factorType)->GetString();
				if (subStr != thisStr)
				{
					match = false;
					break;
				}
			}
			if (!match)
				break;
		}

		if (match)
			return true;
	}
	return false;
}

bool Phrase::IsCompatible(const Phrase &inputPhrase) const
{
	if (inputPhrase.GetSize() != GetSize())
	{
		return false;
	}

	const size_t size = GetSize();

	for (size_t currPos = 0 ; currPos < size ; currPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(currPos, factorType)
									,*inputFactor	= inputPhrase.GetFactor(currPos, factorType);
			if (thisFactor != NULL && inputFactor != NULL && thisFactor != inputFactor)
				return false;
		}
	}
	return true;

}
//
//bool Phrase::Contains(const Phrase &subPhrase) const
//{
//	const size_t subSize = subPhrase.GetSize()
//							,thisSize= GetSize();
//	if (subSize > thisSize)
//		return false;
//
//	// try to match word-for-word
//	for (size_t currStartPos = 0 ; currStartPos < (thisSize - subSize + 1) ; currStartPos++)
//	{
//		bool match = true;
//
//		for (size_t currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
//		{
//			FactorType factorType = static_cast<FactorType>(currFactor);
//			// curr factor, 1st word
//			const Factor *subFactor			= subPhrase.GetFactor(0, factorType)
//										,*thisFactor	= GetFactor(currStartPos, factorType);
//			if (subFactor != NULL && thisFactor != NULL)
//			{
//				if (subFactor != thisFactor)
//				{
//					match = false;
//					break;
//				}
//				else
//				{
//					// subsequent words
//					for (size_t currSubPos = 1 ; currSubPos < subSize ; currSubPos++)
//					{
//						size_t currThisPos = currSubPos + currStartPos;
//						const Factor *subFactor			= subPhrase.GetFactor(currSubPos, factorType)
//													,*thisFactor	= GetFactor(currThisPos, factorType);
//						if (subFactor != thisFactor)
//						{
//							match = false;
//							break;
//						}
//					}
//					if (!match)
//						break;
//				}
//			}
//		}
//
//		if (match)
//			return true;
//	}
//	return false;
//}

// friend
ostream& operator<<(ostream& out, const Phrase& phrase)
{
//	out << "(size " << phrase.GetSize() << ") ";
	for (size_t pos = 0 ; pos < phrase.GetSize() ; pos++)
	{
		const FactorArray &factorArray = phrase.GetFactorArray(pos);
		out << Word::ToString(factorArray);
	}
	return out;
}

