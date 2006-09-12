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

#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "WordsRange.h"

using namespace std;

void PhraseDictionary::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, FactorCollection &factorCollection
																			, const string &filePath
																			, const string &hashFilePath
																			, const vector<float> &weight
																			, size_t tableLimit
																			, bool filter_REMOVE
																			, const list< Phrase > &inputPhraseList
																			, const LMList &languageModels
														          , float weightWP
														          , const StaticData& staticData)
{
	m_tableLimit = tableLimit;
	m_filename = filePath;

	//factors	
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);
	VERBOSE(2,"PhraseDictionary: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);

	// data from file
	InputFileStream inFile(filePath);

	// create hash file if necessary
	ofstream tempFile;
	string tempFilePath;

	vector< vector<string> >	phraseVector;
	string line, prevSourcePhrase = "";
	size_t count = 0;
  size_t line_num = 0;
	while(getline(inFile, line)) 
	{
		++line_num;
		vector<string> tokens = TokenizeMultiCharSeparator( line , "|||" );
		if (tokens.size() != 3)
		{
			TRACE_ERR("Syntax error at " << filePath << ":" << line_num);
			abort(); // TODO- error handling
		}

		bool isLHSEmpty = (tokens[1].find_first_not_of(" \t", 0) == string::npos);
		if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
			TRACE_ERR(filePath << ":" << line_num << ": pt entry contains empty target, skipping\n");
			continue;
		}

		const std::string& factorDelimiter = StaticData::Instance()->GetFactorDelimiter();
		if (tokens[0] != prevSourcePhrase)
			phraseVector = Phrase::Parse(tokens[0], input, factorDelimiter);

		vector<float> scoreVector = Tokenize<float>(tokens[2]);
		if (scoreVector.size() != m_noScoreComponent) {
			TRACE_ERR("Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_noScoreComponent<<") of score components on line " << line_num);
			abort();
		}
//		assert(scoreVector.size() == m_noScoreComponent);
			
		// source
		Phrase sourcePhrase(Input);
		sourcePhrase.CreateFromString( input, phraseVector, factorCollection);
		//target
		TargetPhrase targetPhrase(Output);
		targetPhrase.CreateFromString( output, tokens[1], factorCollection, factorDelimiter);

		// component score, for n-best output
		std::vector<float> scv(scoreVector.size());
		std::transform(scoreVector.begin(),scoreVector.end(),scv.begin(),TransformScore);
		targetPhrase.SetScore(this, scv, weight, weightWP, languageModels);

		AddEquivPhrase(sourcePhrase, targetPhrase);

		count++;
	}

	SortTargetPhraseCollection();
	// sort each target phrase collection
	m_collection.Sort(m_tableLimit);
}

TargetPhraseCollection *PhraseDictionary::CreateTargetPhraseCollection(const Phrase &source)
{
	const size_t size = source.GetSize();
	
	PhraseDictionaryNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		Word word(source.GetFactorArray(pos));
		currNode = currNode->GetOrCreateChild(word);
		if (currNode == NULL)
			return NULL;
	}

	return currNode->CreateTargetPhraseCollection();
}

void PhraseDictionary::AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)
{
	TargetPhraseCollection &phraseColl = *CreateTargetPhraseCollection(source);
	phraseColl.Add(new TargetPhrase(targetPhrase));
}

const TargetPhraseCollection *PhraseDictionary::GetTargetPhraseCollection(const Phrase &source) const
{ // exactly like CreateTargetPhraseCollection, but don't create
	const size_t size = source.GetSize();
	
	const PhraseDictionaryNode *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		Word word(source.GetFactorArray(pos));
		currNode = currNode->GetChild(word);
		if (currNode == NULL)
			return NULL;
	}

	return currNode->GetTargetPhraseCollection();
}

PhraseDictionary::~PhraseDictionary()
{
}

void PhraseDictionary::SetWeightTransModel(const vector<float> &weightT)
{
	PhraseDictionaryNode::iterator iterDict;
	for (iterDict = m_collection.begin() ; iterDict != m_collection.end() ; ++iterDict)
	{
		PhraseDictionaryNode &phraseDictionaryNode = iterDict->second;
		// recursively set weights in nodes
		phraseDictionaryNode.SetWeightTransModel(this, weightT);
	}
}

bool PhraseDictionary::Contains(const vector< vector<string> > &phraseVector
															, const list<Phrase> &inputPhraseList
															, const vector<FactorType> &inputFactorType)
{
	std::list<Phrase>::const_iterator	iter;
	for (iter = inputPhraseList.begin() ; iter != inputPhraseList.end() ; ++iter)
	{
		const Phrase &inputPhrase = *iter;
		if (inputPhrase.Contains(phraseVector, inputFactorType))
			return true;
	}
	return false;
}

void PhraseDictionary::SortTargetPhraseCollection()
{
	m_collection.Sort(m_tableLimit);
}

TO_STRING_BODY(PhraseDictionary);

// friend
ostream& operator<<(ostream& out, const PhraseDictionary& phraseDict)
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

