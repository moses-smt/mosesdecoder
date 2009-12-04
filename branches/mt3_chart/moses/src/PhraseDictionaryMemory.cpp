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
#include "ChartRuleCollection.h"
#include "DotChart.h"

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

inline void TransformString(vector< vector<string>* > &phraseVector
										 ,vector<size_t> &align)
{
	for (size_t pos = 0 ; pos < phraseVector.size() ; ++pos)
	{
		//assert(phraseVector[pos]->size() == 1);

		string &str = (*phraseVector[pos])[0];
		if (str.size() > 3 && str.substr(0, 1) == "[" && str.substr(2, 1) == ",")
		{ // non-term
			string indStr = str.substr(3, str.size() - 4);
			size_t indAlign = Scan<size_t>(indStr) - 1;
			StoreAlign(align, indAlign, pos);

			str = str.substr(1, 1);
		}
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
	m_tableLimit = tableLimit;

	//factors
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);

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
	const std::string& factorDelimiter = staticData.GetFactorDelimiter();

	VERBOSE(2,"PhraseDictionaryMemory: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);

	string line;
	size_t count = 0;
  size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info

	// cache
	TargetPhraseCollection	*prevPhraseColl = NULL;
	vector<size_t> sourceAlign;
	string prevSourcePhraseString;

	vector<string> tokens;
	vector<float> scoreVector;
	vector< vector<string>* >	sourcePhraseVector;
	vector< vector<string>* >	targetPhraseVector;
	list<pair<size_t,size_t> > alignmentInfo;

	while(getline(inStream, line))
	{
		tokens.clear();
		TokenizeMultiCharSeparator(tokens, line , "|||" );

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

		string &headString					= tokens[0]
					, &sourcePhraseString	= tokens[1]
					, &targetPhraseString	= tokens[2]
					, &scoreString				= tokens[3];

		bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
		if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
			TRACE_ERR( m_filePath << ":" << count << ": pt entry contains empty target, skipping\n");
			continue;
		}

		scoreVector.clear();
		Tokenize<float>(scoreVector, scoreString);
		if (scoreVector.size() != m_numScoreComponent)
		{
			stringstream strme;
			strme << "Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_numScoreComponent<<") of score components on line " << count;
			UserMessage::Add(strme.str());
			abort();
		}
		assert(scoreVector.size() == m_numScoreComponent);

		if (prevSourcePhraseString == sourcePhraseString)
		{ // same source, use cached source phrase & target phrase coll
			// do nothing
		}
		else
		{ // parse source & find pt node
			prevSourcePhraseString = sourcePhraseString;

			// parse
			sourcePhraseVector.clear();
			Phrase::Parse(sourcePhraseVector, sourcePhraseString, input, factorDelimiter);

			// align info
			sourceAlign.clear();
			TransformString(sourcePhraseVector, sourceAlign);

			// create phrase obj
			Phrase sourcePhrase(Input, sourcePhraseVector.size());
			sourcePhrase.CreateFromString( input, sourcePhraseVector);

			prevPhraseColl = &GetOrCreateTargetPhraseCollection(sourcePhrase);

			RemoveAllInColl(sourcePhraseVector);
		}

		// parse target phrase
		targetPhraseVector.clear();
		Phrase::Parse(targetPhraseVector, targetPhraseString, output, factorDelimiter);

		// alignment
		alignmentInfo.clear();
		vector<size_t> targetAlign;
		TransformString(targetPhraseVector, targetAlign);

		assert(sourceAlign.size() == targetAlign.size());
		for (size_t ind = 0; ind < targetAlign.size(); ++ind)
		{
			alignmentInfo.push_back(pair<size_t,size_t>(sourceAlign[ind], targetAlign[ind]));
		}

		// head word
		headString = headString.substr(1, headString.size() - 2);
		Word headWord;
		headWord.CreateFromString(Input, input,headString);

		// create target phrase obj
		TargetPhrase *targetPhrase = new TargetPhrase(Output, targetPhraseVector.size());
		//targetPhrase->SetSourcePhrase(sourcePhrase); // TODO not valid
		targetPhrase->CreateFromString( output, targetPhraseVector);

		targetPhrase->SetAlignmentInfo(alignmentInfo);
		targetPhrase->SetTargetLHS(headWord);

		targetPhrase->SetDebugOutput(string("Mem pt " )+ line);

		// remove strings
		RemoveAllInColl(targetPhraseVector);

		// component score, for n-best output
		std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),NegateScore);
		std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);

		if (GetPhraseTableImplementation() == GlueRule)
		{
			targetPhrase->SetScoreChart(this, scoreVector, weight, languageModels, false);
		}
		else
		{
			targetPhrase->SetScoreChart(this, scoreVector, weight, languageModels, true);
		}

		AddEquivPhrase(*prevPhraseColl, targetPhrase);

		count++;
	}

	// cleanup cache

	// sort each target phrase collection
	m_collection.Sort(m_tableLimit);

	return true;
}

TargetPhraseCollection &PhraseDictionaryMemory::GetOrCreateTargetPhraseCollection(const Phrase &source)
{
	const size_t size = source.GetSize();

	PhraseDictionaryNodeMemory *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->GetOrCreateChild(word);
		assert(currNode != NULL);
	}

	return currNode->GetOrCreateTargetPhraseCollection();
}

void PhraseDictionaryMemory::AddEquivPhrase(TargetPhraseCollection	&targetPhraseColl, TargetPhrase *targetPhrase)
{
	targetPhraseColl.Add(targetPhrase);
}

const TargetPhraseCollection *PhraseDictionaryMemory::GetTargetPhraseCollection(const Phrase &source) const
{ // exactly like CreateTargetPhraseCollection, but don't create
	const size_t size = source.GetSize();

	const PhraseDictionaryNodeMemory *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->GetChild(word);
		if (currNode == NULL)
			return NULL;
	}

	return currNode->GetTargetPhraseCollection();
}

void PhraseDictionaryMemory::InitializeForInput(const InputType& input)
{
	assert(m_runningNodesVec.size() == 0);
	size_t sourceSize = input.GetSize();
	m_runningNodesVec.resize(sourceSize);

	for (size_t ind = 0; ind < m_runningNodesVec.size(); ++ind)
	{
		ProcessedRule *initProcessedRule = new ProcessedRule(m_collection);

		ProcessedRuleStack *processedStack = new ProcessedRuleStack(sourceSize - ind + 1);
		processedStack->Add(0, initProcessedRule); // init rule. stores the top node in tree

		m_runningNodesVec[ind] = processedStack;
	}
}

PhraseDictionaryMemory::~PhraseDictionaryMemory()
{
	CleanUp();
}

void PhraseDictionaryMemory::SetWeightTransModel(const vector<float> &weightT)
{
	PhraseDictionaryNodeMemory::iterator iterDict;
	for (iterDict = m_collection.begin() ; iterDict != m_collection.end() ; ++iterDict)
	{
		PhraseDictionaryNodeMemory &phraseDictionaryNode = iterDict->second;
		// recursively set weights in nodes
		phraseDictionaryNode.SetWeightTransModel(this, weightT);
	}
}


void PhraseDictionaryMemory::CleanUp()
{
	//RemoveAllInColl(m_chartTargetPhraseColl);
	std::vector<ChartRuleCollection*>::iterator iter;
	for (iter = m_chartTargetPhraseColl.begin(); iter != m_chartTargetPhraseColl.end(); ++iter)
	{
		ChartRuleCollection *item = *iter;
		ChartRuleCollection::Delete(item);
	}
	m_chartTargetPhraseColl.clear();

	RemoveAllInColl(m_runningNodesVec);
}

TO_STRING_BODY(PhraseDictionaryMemory);

// friend
ostream& operator<<(ostream& out, const PhraseDictionaryMemory& phraseDict)
{
	const PhraseDictionaryNodeMemory &coll = phraseDict.m_collection;
	PhraseDictionaryNodeMemory::const_iterator iter;
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		const Word &word = (*iter).first;
		out << word;
	}
	return out;
}


}

