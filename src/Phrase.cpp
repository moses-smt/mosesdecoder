 // $Id$
// vim:tabstop=2

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

#include <cassert>
#include <algorithm>
#include <sstream>
#include <string>
#include "memory.h"
#include "FactorCollection.h"
#include "Phrase.h"
#include "StaticData.h"  // GetMaxNumFactors

using namespace std;

Phrase::Phrase(const Phrase &copy)
:m_direction(copy.m_direction)
,m_phraseSize(copy.m_phraseSize)
,m_arraySize(copy.m_arraySize)
,m_words(copy.m_words)
{
}

Phrase& Phrase::operator=(const Phrase& x) 
{
	if(this!=&x)
		{

			m_direction=x.m_direction;
			m_phraseSize=x.m_phraseSize;
			m_arraySize=x.m_arraySize;

			m_words = x.m_words;
		}
	return *this;
}


Phrase::Phrase(FactorDirection direction)
	: m_direction(direction)
	, m_phraseSize(0)
	, m_arraySize(ARRAY_SIZE_INCR)
	, m_words(ARRAY_SIZE_INCR)
{
}

Phrase::Phrase(FactorDirection direction, const vector< const Word* > &mergeWords)
:m_direction(direction)
,m_phraseSize(0)
,m_arraySize(ARRAY_SIZE_INCR)
,m_words(ARRAY_SIZE_INCR)
{
	for (size_t currPos = 0 ; currPos < mergeWords.size() ; currPos++)
	{
		AddWord(*mergeWords[currPos]);
	}
}

Phrase::~Phrase()
{
}

void Phrase::MergeFactors(const Phrase &copy)
{
	assert(GetSize() == copy.GetSize());
	size_t size = GetSize();
	const size_t maxNumFactors = StaticData::Instance()->GetMaxNumFactors(this->GetDirection());
	for (size_t currPos = 0 ; currPos < size ; currPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < maxNumFactors ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *factor = copy.GetFactor(currPos, factorType);
			if (factor != NULL)
				SetFactor(currPos, factorType, factor);
		}
	}
}

void Phrase::MergeFactors(const Phrase &copy, FactorType factorType)
{
	assert(GetSize() == copy.GetSize());
	for (size_t currPos = 0 ; currPos < GetSize() ; currPos++)
			SetFactor(currPos, factorType, copy.GetFactor(currPos, factorType));
}

void Phrase::MergeFactors(const Phrase &copy, const std::vector<FactorType>& factorVec)
{
	assert(GetSize() == copy.GetSize());
	for (size_t currPos = 0 ; currPos < GetSize() ; currPos++)
		for (std::vector<FactorType>::const_iterator i = factorVec.begin();
		     i != factorVec.end(); ++i)
		{
			SetFactor(currPos, *i, copy.GetFactor(currPos, *i));
		}
}


Phrase Phrase::GetSubString(const WordsRange &wordsRange) const
{
	Phrase retPhrase(m_direction);

	for (size_t currPos = wordsRange.GetStartPos() ; currPos <= wordsRange.GetEndPos() ; currPos++)
	{
		Word &word = retPhrase.AddWord();
		word = GetWord(currPos);
	}

	return retPhrase;
}

std::string Phrase::GetStringRep(const vector<FactorType> factorsToPrint) const
{
	Phrase retPhrase(m_direction);
	stringstream strme;
	for (size_t pos = 0 ; pos < GetSize() ; pos++)
	{
		strme << GetWord(pos).ToString(factorsToPrint);
	}

	return strme.str();
}

Word &Phrase::AddWord()
{
	if ((m_phraseSize+1) % ARRAY_SIZE_INCR == 0)
	{ // need to expand array
		m_arraySize += ARRAY_SIZE_INCR;
		m_words.resize(m_arraySize);
	}

	return m_words[m_phraseSize++];
}

void Phrase::Append(const Phrase &endPhrase){
	
	for (int i = 0; i < endPhrase.GetSize();i++){
		AddWord(endPhrase.GetWord(i));	
	}
}

vector< vector<string> > Phrase::Parse(const std::string &phraseString, const std::vector<FactorType> &factorOrder, const std::string& factorDelimiter)
{
	bool isMultiCharDelimiter = factorDelimiter.size() > 1;
	// parse
	vector< vector<string> > phraseVector;
	vector<string> annotatedWordVector = Tokenize(phraseString);
	// KOMMA|none ART|Def.Z NN|Neut.NotGen.Sg VVFIN|none 
	//		to
	// "KOMMA|none" "ART|Def.Z" "NN|Neut.NotGen.Sg" "VVFIN|none"

	for (size_t phrasePos = 0 ; phrasePos < annotatedWordVector.size() ; phrasePos++)
	{
		string &annotatedWord = annotatedWordVector[phrasePos];
		vector<string> factorStrVector;
		if (isMultiCharDelimiter) {
			factorStrVector = TokenizeMultiCharSeparator(annotatedWord, factorDelimiter);
		} else {
			factorStrVector = Tokenize(annotatedWord, factorDelimiter);
		}
		// KOMMA|none
		//    to
		// "KOMMA" "none"
		if (factorStrVector.size() != factorOrder.size()) {
			std::cerr << "[ERROR] Malformed input at " << /*StaticData::Instance()->GetCurrentInputPosition() <<*/ std::endl
			          << "  Expected input to have words composed of " << factorOrder.size() << " factor(s) (form FAC1|FAC2|...)" << std::endl
								<< "  but instead received input with " << factorStrVector.size() << " factor(s).\n";
			abort();
		}
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
		Word &word = AddWord();
		for (size_t currFactorIndex= 0 ; currFactorIndex < factorOrder.size() ; currFactorIndex++)
		{
			FactorType factorType = factorOrder[currFactorIndex];
			const string &factorStr = phraseVector[phrasePos][currFactorIndex];
			const Factor *factor = factorCollection.AddFactor(m_direction, factorType, factorStr); 
			word[factorType] = factor;
		}
	}
}

void Phrase::CreateFromString(const std::vector<FactorType> &factorOrder
															, const string &phraseString
															, FactorCollection &factorCollection
		, const string &factorDelimiter)
{
	vector< vector<string> > phraseVector = Parse(phraseString, factorOrder, factorDelimiter);
	CreateFromString(factorOrder, phraseVector, factorCollection);
}

bool Phrase::operator < (const Phrase &compare) const
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

		const size_t maxNumFactors = StaticData::Instance()->GetMaxNumFactors(this->GetDirection());
		// taken from word.Compare()
		for (size_t i = 0 ; i < maxNumFactors ; i++)
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

	const size_t maxNumFactors = StaticData::Instance()->GetMaxNumFactors(this->GetDirection());
	for (size_t currPos = 0 ; currPos < size ; currPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < maxNumFactors ; currFactor++)
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

bool Phrase::IsCompatible(const Phrase &inputPhrase, FactorType factorType) const
{
	if (inputPhrase.GetSize() != GetSize())	{ return false;	}
	for (size_t currPos = 0 ; currPos < GetSize() ; currPos++)
	{
		if (GetFactor(currPos, factorType) != inputPhrase.GetFactor(currPos, factorType))
			return false;
	}
	return true;
}

bool Phrase::IsCompatible(const Phrase &inputPhrase, const std::vector<FactorType>& factorVec) const
{
	if (inputPhrase.GetSize() != GetSize())	{ return false;	}
	for (size_t currPos = 0 ; currPos < GetSize() ; currPos++)
	{
		for (std::vector<FactorType>::const_iterator i = factorVec.begin();
		     i != factorVec.end(); ++i)
		{
			if (GetFactor(currPos, *i) != inputPhrase.GetFactor(currPos, *i))
				return false;
		}
	}
	return true;
}

void Phrase::InitializeMemPool()
{
}

void Phrase::FinalizeMemPool()
{
}

TO_STRING_BODY(Phrase);

// friend
ostream& operator<<(ostream& out, const Phrase& phrase)
{
//	out << "(size " << phrase.GetSize() << ") ";
	for (size_t pos = 0 ; pos < phrase.GetSize() ; pos++)
	{
		const Word &word = phrase.GetWord(pos);
		out << word;
	}
	return out;
}

