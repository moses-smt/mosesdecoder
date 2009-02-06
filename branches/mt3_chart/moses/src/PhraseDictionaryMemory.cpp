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

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include "PhraseDictionaryMemory.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"
#include "UserMessage.h"
#include "AlignmentPair.h"
#include "ChartRuleCollection.h"

using namespace std;

namespace Moses
{

inline void StoreAlign(vector<size_t> &align, size_t ind, size_t pos)
{ 
	if (ind >= align.size())
	{
		align.resize(ind + 1);
	}
	align[ind] = pos;
}

inline void TransformString(vector< vector<string> > &phraseVector
										 ,vector<size_t> &align)
{
	for (size_t pos = 0 ; pos < phraseVector.size() ; ++pos)
	{
		assert(phraseVector[pos].size() == 1);

		string &str = phraseVector[pos][0];
		if (str.size() > 3 && str.substr(0, 3) == "[X,")
		{ // non-term
			string indStr = str.substr(3, str.size() - 4);
			size_t indAlign = Scan<size_t>(indStr) - 1;
			StoreAlign(align, indAlign, pos);

			str = "[X]";
		}
	}
}

inline void TransformString(vector< vector<string> > &sourcePhraseVector
										 ,vector< vector<string> > &targetPhraseVector
										 ,list<pair<size_t,size_t> > &alignmentInfo)
{
	vector<size_t> sourceAlign, targetAlign;
	TransformString(sourcePhraseVector, sourceAlign);
	TransformString(targetPhraseVector, targetAlign);

	assert(sourceAlign.size() == targetAlign.size());
	for (size_t ind = 0; ind < targetAlign.size(); ++ind)
	{
		alignmentInfo.push_back(pair<size_t,size_t>(sourceAlign[ind], targetAlign[ind]));
	}	
}

bool PhraseDictionaryMemory::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, const string &filePath
																			, const vector<float> &weight
																			, size_t tableLimit
																			, const LMList &languageModels
														          , float weightWP)
{
	m_filePath = filePath;

	// data from file
	InputFileStream inFile(filePath);
	return Load(input, output, inFile, weight, tableLimit, languageModels, weightWP);

}


bool PhraseDictionaryMemory::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, std::istream &inStream
																			, const std::vector<float> &weight
																			, size_t tableLimit
																			, const LMList &languageModels
																			, float weightWP)
{
	PrintUserTime("Start loading tm");

	const StaticData &staticData = StaticData::Instance();
	m_tableLimit = tableLimit;

	//factors
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);
	VERBOSE(2,"PhraseDictionaryMemory: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);

	// create hash file if necessary
	ofstream tempFile;
	string tempFilePath;

	string line;
	size_t count = 0;
  size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info

	while(getline(inStream, line))
	{
		vector<string> tokens = TokenizeMultiCharSeparator( line , "|||" );

		if (numElement == NOT_FOUND)
		{ // init numElement
			numElement = tokens.size();
			assert(numElement == 4);
		}

		if (tokens.size() != numElement)
		{
			stringstream strme;
			strme << "Syntax error at " << m_filePath << ":" << count;
			UserMessage::Add(strme.str());
			abort();
		}

		assert(Trim(tokens[0]) == "[X]");
		string sourcePhraseString	=tokens[1]
					, targetPhraseString=tokens[2]
					, scoreString				=tokens[3];

		bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
		if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
			TRACE_ERR( m_filePath << ":" << count << ": pt entry contains empty target, skipping\n");
			continue;
		}

		vector<float> scoreVector = Tokenize<float>(scoreString);
		if (scoreVector.size() != m_numScoreComponent)
		{
			stringstream strme;
			strme << "Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_numScoreComponent<<") of score components on line " << count;
			UserMessage::Add(strme.str());
			abort();
		}
		assert(scoreVector.size() == m_numScoreComponent);

		const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
		vector< vector<string> >	sourcePhraseVector = Phrase::Parse(sourcePhraseString, input, factorDelimiter)
															,targetPhraseVector = Phrase::Parse(targetPhraseString, output, factorDelimiter);
		list<pair<size_t,size_t> > alignmentInfo;
		TransformString(sourcePhraseVector, targetPhraseVector, alignmentInfo);

		// source
		Phrase sourcePhrase(Input);
		sourcePhrase.CreateFromString( input, sourcePhraseVector);

		//target
		TargetPhrase *targetPhrase = new TargetPhrase(Output);
		targetPhrase->SetSourcePhrase(&sourcePhrase);
		targetPhrase->CreateFromString( output, targetPhraseVector);

		targetPhrase->CreateAlignmentInfo(alignmentInfo);

		// component score, for n-best output
		std::vector<float> scv(scoreVector.size());
		std::transform(scoreVector.begin(),scoreVector.end(),scv.begin(),NegateScore);

		std::transform(scv.begin(),scv.end(),scv.begin(),FloorScore);
		targetPhrase->SetScore(this, scv, weight, weightWP, languageModels);

		AddEquivPhrase(sourcePhrase, targetPhrase);

		if (count % 10000 == 0)
			cerr << count << " " << line << endl;

		count++;
	}

	PrintUserTime("Finished loading tm");

	// sort each target phrase collection
	m_collection.Sort(m_tableLimit);

	PrintUserTime("Finished sorting tm");

	return true;
}

TargetPhraseCollection &PhraseDictionaryMemory::GetOrCreateTargetPhraseCollection(const Phrase &source)
{
	const size_t size = source.GetSize();

	PhraseDictionaryNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->GetOrCreateChild(word);
		assert(currNode != NULL);
	}

	return currNode->GetOrCreateTargetPhraseCollection();
}

void PhraseDictionaryMemory::AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase)
{
	if (m_prevPhraseColl != NULL && source.Compare(m_prevSource) == 0)
	{ // same source, already have node
	}
	else
	{
		m_prevPhraseColl = &GetOrCreateTargetPhraseCollection(source);
		m_prevSource = source;
	}

	m_prevPhraseColl->Add(targetPhrase);
}

const TargetPhraseCollection *PhraseDictionaryMemory::GetTargetPhraseCollection(const Phrase &source) const
{ // exactly like CreateTargetPhraseCollection, but don't create
	const size_t size = source.GetSize();

	const PhraseDictionaryNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->GetChild(word);
		if (currNode == NULL)
			return NULL;
	}

	return currNode->GetTargetPhraseCollection();
}

PhraseDictionaryMemory::~PhraseDictionaryMemory()
{
	CleanUp();
}

void PhraseDictionaryMemory::SetWeightTransModel(const vector<float> &weightT)
{
	PhraseDictionaryNode::iterator iterDict;
	for (iterDict = m_collection.begin() ; iterDict != m_collection.end() ; ++iterDict)
	{
		PhraseDictionaryNode &phraseDictionaryNode = iterDict->second;
		// recursively set weights in nodes
		phraseDictionaryNode.SetWeightTransModel(this, weightT);
	}
}


void PhraseDictionaryMemory::CleanUp()
{
	RemoveAllInColl(m_chartTargetPhraseColl);
}

TO_STRING_BODY(PhraseDictionaryMemory);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryMemory& phraseDict)
{
	const PhraseDictionaryNode &coll = phraseDict.m_collection;
	PhraseDictionaryNode::const_iterator iter;
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		const Word &word = (*iter).first;
		out << word;
	}
	return out;
}


}

