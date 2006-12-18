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
#include "PhraseCollection.h"

using namespace std;

bool PhraseDictionaryMemory::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, FactorCollection &factorCollection
																			, const string &filePath
																			, const vector<float> &weight
																			, size_t tableLimit
																			, const LMList &languageModels
														          , float weightWP
														          , const StaticData& staticData
																			, const PhraseCollection *inputPhrases)
{
	m_tableLimit = tableLimit;
	m_filePath = filePath;

	//factors	
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);
	VERBOSE(2,"PhraseDictionaryMemory: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);

	// data from file
	InputFileStream inFile(filePath);

	// create hash file if necessary
	ofstream tempFile;
	string tempFilePath;

	string line, prevSourcePhrase = "";
	size_t count = 0;
  size_t line_num = 0;
	while(getline(inFile, line)) 
	{
		++line_num;
		vector<string> tokens = TokenizeMultiCharSeparator( line , "|||" );
		if (tokens.size() != 3)
		{
			stringstream strme;
			strme << "Syntax error at " << filePath << ":" << line_num;
			UserMessage::Add(strme.str());
			return false;
		}

		bool isLHSEmpty = (tokens[1].find_first_not_of(" \t", 0) == string::npos);
		if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
			TRACE_ERR( filePath << ":" << line_num << ": pt entry contains empty target, skipping\n");
			continue;
		}

		const std::string& factorDelimiter = StaticData::Instance()->GetFactorDelimiter();
		
		// only have to recreate source phrase if different from prev
		//if (tokens[0] != prevSourcePhrase)
		//	phraseVector = Phrase::Parse(tokens[0], input, factorDelimiter);

		vector<float> scoreVector = Tokenize<float>(tokens[2]);
		if (scoreVector.size() != m_numScoreComponent) 
		{
			stringstream strme;
			strme << "Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_numScoreComponent<<") of score components on line " << line_num;
			UserMessage::Add(strme.str());
			return false;
		}
			
		TargetPhrase targetPhrase(Output);

		// alignment info
		AlignmentPair &alignmentPair = targetPhrase.GetAlignmentPair();

		// source
		Phrase sourcePhrase(Input);
		sourcePhrase.CreateFromString( input
																, tokens[0]
																, factorCollection
																, factorDelimiter
																, &alignmentPair.GetInserter(Input));

		// if not part of input, filter it out
		if (inputPhrases != NULL && !inputPhrases->Find(sourcePhrase, false))
		{
			continue;
		}

		//target
		targetPhrase.CreateFromString( output
																, tokens[1]
																, factorCollection
																, factorDelimiter
																, &alignmentPair.GetInserter(Output));
		
		// component score, for n-best output
		std::vector<float> scv(scoreVector.size());
		std::transform(scoreVector.begin(),scoreVector.end(),scv.begin(),TransformScore);
		std::transform(scv.begin(),scv.end(),scv.begin(),FloorScore);
		targetPhrase.SetScore(this, scv, weight, weightWP, languageModels);

		AddEquivPhrase(sourcePhrase, targetPhrase);

		count++;
	}

	// sort each target phrase collection
	m_collection.Sort(m_tableLimit);

	return true;
}

TargetPhraseCollection *PhraseDictionaryMemory::CreateTargetPhraseCollection(const Phrase &source)
{
	const size_t size = source.GetSize();
	
	PhraseDictionaryNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->GetOrCreateChild(word);
		if (currNode == NULL)
			return NULL;
	}

	return currNode->CreateTargetPhraseCollection();
}

void PhraseDictionaryMemory::AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)
{
	TargetPhraseCollection &phraseColl = *CreateTargetPhraseCollection(source);
	phraseColl.Add(new TargetPhrase(targetPhrase));
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

