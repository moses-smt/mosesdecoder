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
#include "PhraseDictionarySourceLabel.h"
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

inline void TransformString(vector< vector<string>* > &phraseVector)
{ // for target phrase
	for (size_t pos = 0 ; pos < phraseVector.size() ; ++pos)
	{
		assert(phraseVector[pos]->size() == 1);

		string &str = (*phraseVector[pos])[0];
		if (str.substr(0, 1) == "[" && str.substr(str.size()-1, 1) == "]")
		{ // non-term
			str = str.substr(1, str.size() - 2);
		}
	}
}

inline void TransformString(vector< vector<string>* > &phraseVector
										, vector<string> &sourceLabelsStr)
{ // for source phrase
	for (size_t pos = 0 ; pos < phraseVector.size() ; ++pos)
	{
		assert(phraseVector[pos]->size() == 1);

		string &str = (*phraseVector[pos])[0];
		if (str.substr(0, 1) == "[" && str.substr(str.size()-1, 1) == "]")
		{ // non-term
			string sourceLabel = str.substr(1, str.size() - 2);
			sourceLabelsStr.push_back(sourceLabel);
			str = "X"; // TODO
		}
	}
}

inline void CreateAlignmentMapping(vector<size_t> &mapping
															, const Phrase &phrase)
{ // alignments are between non-terms, but should be between all words. 
	// should change extraction procedure so we won't need this
	for (size_t pos = 0; pos < phrase.GetSize(); ++pos)
	{
		const Word &word = phrase.GetWord(pos);
		if (word.IsNonTerminal())
			mapping.push_back(pos);
	}
}

void PhraseDictionarySourceLabel::CreateSourceLabels(vector<Word> &sourceLabels
															, const vector<string> &sourceLabelsStr) const
{
	FactorCollection &factorCollection = FactorCollection::Instance();

	for (size_t ind = 0; ind < sourceLabelsStr.size(); ++ind)
	{
		sourceLabels.push_back(Word());
		Word &word = sourceLabels.back();
		
		// TODO
		const Factor *factor = factorCollection.AddFactor(Input, 0, sourceLabelsStr[ind], true);
		word[0] = factor;
		word.SetIsNonTerminal(true);
	}
}

bool PhraseDictionarySourceLabel::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, const string &filePath
																			, const string &fileBackoffPath
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
	InputFileStream *inBackoffFile = NULL;
	
	if (fileBackoffPath != "")
	{
		inBackoffFile = new InputFileStream(fileBackoffPath);
	}

	bool ret = Load(input, output, inFile, inBackoffFile, weight, tableLimit, languageModels, weightWP);
	
	delete inBackoffFile;
	return ret;
}

bool PhraseDictionarySourceLabel::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, std::istream &inStream
																		  , std::istream *inBackoffStream
																			, const std::vector<float> &weight
																			, size_t tableLimit
																			, const LMList &languageModels
																			, float weightWP)
{
	PrintUserTime("Start loading reordering model");

	const StaticData &staticData = StaticData::Instance();
	const std::string& factorDelimiter = staticData.GetFactorDelimiter();

	VERBOSE(2,"PhraseDictionarySourceLabel: input=" << m_inputFactors << "  output=" << m_outputFactors << std::endl);

	string line;
	size_t count = 0;
  size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info


	while(getline(inStream, line))
	{
		TargetPhraseCollection	*prevPhraseColl = NULL;
		
		vector<string> tokens;
		vector<float> scoreVector;
		vector< vector<string>* >	sourcePhraseVector;
		vector< vector<string>* >	targetPhraseVector;
		vector<string> sourceLabelsStr;
		list<pair<size_t,size_t> > alignmentInfo;

		TokenizeMultiCharSeparator(tokens, line , "|||" );

		if (numElement == NOT_FOUND)
		{ // init numElement
			numElement = tokens.size();
			assert(numElement == 5);
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
					, &alignString				= tokens[3]
					, &scoreString				= tokens[4];

		bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
		if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
			TRACE_ERR( m_filePath << ":" << count << ": pt entry contains empty target, skipping\n");
			continue;
		}

		Tokenize<float>(scoreVector, scoreString);
		if (scoreVector.size() != m_numScoreComponent)
		{
			stringstream strme;
			strme << "Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_numScoreComponent<<") of score components on line " << count;
			UserMessage::Add(strme.str());
			abort();
		}
		assert(scoreVector.size() == m_numScoreComponent);

		// parse source & find pt node

		// parse
		Phrase::Parse(sourcePhraseVector, sourcePhraseString, input, factorDelimiter);

		// source info
		TransformString(sourcePhraseVector, sourceLabelsStr);

		// create phrase obj
		Phrase sourcePhrase(Input, sourcePhraseVector.size());
		sourcePhrase.CreateFromString( input, sourcePhraseVector);

		vector<Word> sourceLabels;
		CreateSourceLabels(sourceLabels, sourceLabelsStr);

		prevPhraseColl = &GetOrCreateTargetPhraseCollection(sourcePhrase, sourceLabels);

		RemoveAllInColl(sourcePhraseVector);

		// parse target phrase
		targetPhraseVector.clear();
		Phrase::Parse(targetPhraseVector, targetPhraseString, output, factorDelimiter);
		TransformString(targetPhraseVector);
		TargetPhrase *targetPhrase = new TargetPhrase(Output, targetPhraseVector.size());
		//targetPhrase->SetSourcePhrase(sourcePhrase); // TODO not valid
		targetPhrase->CreateFromString( output, targetPhraseVector);

		// alignment
		vector<size_t> sourceAlignMap, targetAlignMap;
		CreateAlignmentMapping(sourceAlignMap, sourcePhrase);
		CreateAlignmentMapping(targetAlignMap, *targetPhrase);

		vector<string>	alignVecStr = Tokenize<string>(alignString);
		for (size_t ind = 0; ind < alignVecStr.size(); ++ind)
		{
			vector<size_t> alignVec = Tokenize<size_t>(alignVecStr[ind], "-");
			assert(alignVec.size() == 2);
			alignmentInfo.push_back(pair<size_t,size_t>(sourceAlignMap[ alignVec[0] ], targetAlignMap[ alignVec[1] ]));
		}

		targetPhrase->CreateAlignmentInfo(alignmentInfo);
		
		targetPhrase->SetDebugOutput(string("Source Label pt ") +  line);

		// head word
		headString = headString.substr(1, headString.size() - 2);
		Word headWord;
		headWord.CreateFromString(Input, input,headString);

		targetPhrase->SetHeadWord(headWord);

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

	// add backoff
	bool ret;
	if (inBackoffStream != NULL)
	{
		ret = LoadBackoff(input, output
											 , *inBackoffStream
											 , weight
											 , tableLimit
											 , languageModels
											 , weightWP);
	}
	else
		ret = true;

	// sort each target phrase collection
	m_collection.Sort(m_tableLimit);
	
	return ret;
}

bool PhraseDictionarySourceLabel::LoadBackoff(const std::vector<FactorType> &input
																											 , const std::vector<FactorType> &output
																											 , std::istream &inBackoffStream
																											 , const std::vector<float> &weight
																											 , size_t tableLimit
																											 , const LMList &languageModels
																											 , float weightWP)
{	
	const StaticData &staticData = StaticData::Instance();
	const std::string& factorDelimiter = staticData.GetFactorDelimiter();
	FactorCollection &factorCollection = FactorCollection::Instance();
	
	VERBOSE(1,"Loading backoff\n");
	
	string line;
	size_t count = 0;
  size_t numElement = NOT_FOUND; // 3=old format, 5=async format which include word alignment info
	
	while(getline(inBackoffStream, line))
	{		
		vector<string> tokens;
		vector<float> scoreVector;
		vector< vector<string>* >	sourcePhraseVector;
		vector< vector<string>* >	targetPhraseVector;
		vector<string> sourceLabelsStr;
		list<pair<size_t,size_t> > alignmentInfo;
		
		TokenizeMultiCharSeparator(tokens, line , "|||" );
		
		if (numElement == NOT_FOUND)
		{ // init numElement
			numElement = tokens.size();
			assert(numElement == 8);
		}
		
		if (tokens.size() != numElement)
		{
			stringstream strme;
			strme << "Syntax error at " << m_filePath << ":" << count;
			UserMessage::Add(strme.str());
			abort();
		}
		
		const string 
		 &sourcePhraseString	= tokens[1]
		, &targetPhraseString	= tokens[2]
		, &alignString				= tokens[3]
		, &scoreString				= tokens[4];
				
		string headString		= tokens[0];
		
		size_t coveredWordInd	= Scan<size_t>(tokens[5]);
		float entropy				= Scan<float>(tokens[7]);
		
		bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
		if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
			TRACE_ERR( m_filePath << ":" << count << ": pt entry contains empty target, skipping\n");
			continue;
		}
		
		Tokenize<float>(scoreVector, scoreString);
		if (scoreVector.size() != m_numScoreComponent)
		{
			stringstream strme;
			strme << "Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_numScoreComponent<<") of score components on line " << count;
			UserMessage::Add(strme.str());
			abort();
		}
		assert(scoreVector.size() == m_numScoreComponent);
		
		// parse source & find pt node
		
		// parse
		Phrase::Parse(sourcePhraseVector, sourcePhraseString, input, factorDelimiter);
		
		// source info
		TransformString(sourcePhraseVector, sourceLabelsStr);
		
		// create phrase obj
		Phrase sourcePhrase(Input, sourcePhraseVector.size());
		sourcePhrase.CreateFromString( input, sourcePhraseVector);
		
		vector<Word> sourceLabels;
		CreateSourceLabels(sourceLabels, sourceLabelsStr);
		
		// orig coll with source label
		const PhraseDictionaryNodeSourceLabel &origNode = GetOrCreateNode(sourcePhrase, sourceLabels);
		
		// coll with hacked label
		// parse covered words
		// TODO hack

		string coveredWordsString = tokens[6];
		coveredWordsString = Replace(coveredWordsString, " ", "_");
		coveredWordsString = sourceLabels[coveredWordInd].GetFactor(0)->GetString() + "_" + coveredWordsString;
		const Factor *factor = factorCollection.AddFactor(Input, input[0], coveredWordsString, true);
		Word newWord;
		newWord.SetFactor(input[0], factor);
		sourceLabels[coveredWordInd] = newWord;
				
		PhraseDictionaryNodeSourceLabel &node =  GetOrCreateNode(sourcePhrase, sourceLabels, entropy);
		node.SetId(origNode.GetId());
		TargetPhraseCollection	&prevPhraseColl = node.GetOrCreateTargetPhraseCollection();
		
		RemoveAllInColl(sourcePhraseVector);
		
		// parse target phrase
		targetPhraseVector.clear();
		Phrase::Parse(targetPhraseVector, targetPhraseString, output, factorDelimiter);
		TransformString(targetPhraseVector);
		TargetPhrase *targetPhrase = new TargetPhrase(Output, targetPhraseVector.size());
		//targetPhrase->SetSourcePhrase(sourcePhrase); // TODO not valid
		targetPhrase->CreateFromString( output, targetPhraseVector);
		
		// alignment
		vector<size_t> sourceAlignMap, targetAlignMap;
		CreateAlignmentMapping(sourceAlignMap, sourcePhrase);
		CreateAlignmentMapping(targetAlignMap, *targetPhrase);
		
		vector<string>	alignVecStr = Tokenize<string>(alignString);
		for (size_t ind = 0; ind < alignVecStr.size(); ++ind)
		{
			vector<size_t> alignVec = Tokenize<size_t>(alignVecStr[ind], "-");
			assert(alignVec.size() == 2);
			alignmentInfo.push_back(pair<size_t,size_t>(sourceAlignMap[ alignVec[0] ], targetAlignMap[ alignVec[1] ]));
		}
		
		targetPhrase->CreateAlignmentInfo(alignmentInfo);
		targetPhrase->SetDebugOutput(string("Backoff pt ") +  line);
		
		// head word
		headString = headString.substr(1, headString.size() - 2);
		Word headWord;
		headWord.CreateFromString(Input, input,headString);
		
		targetPhrase->SetHeadWord(headWord);
		
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
		
		AddEquivPhrase(prevPhraseColl, targetPhrase);
		
		count++;
		
	}
	
	return true;
}

TargetPhraseCollection &PhraseDictionarySourceLabel::GetOrCreateTargetPhraseCollection(const Phrase &source
																																		, const std::vector<Word> &sourceLabels)
{
	PhraseDictionaryNodeSourceLabel &currNode = GetOrCreateNode(source, sourceLabels);
	return currNode.GetOrCreateTargetPhraseCollection();
}

PhraseDictionaryNodeSourceLabel &PhraseDictionarySourceLabel::GetOrCreateNode(const Phrase &source
																																		, const std::vector<Word> &sourceLabels
																																		, float entropy)
{
	const size_t size = source.GetSize();
	
	size_t nonTermInd = 0;
	PhraseDictionaryNodeSourceLabel *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		if (word.IsNonTerminal())
		{ // indexed by source label 1st
			currNode = currNode->GetOrCreateChild(word, sourceLabels[nonTermInd]);
			++nonTermInd;
		}
		else
		{
			currNode = currNode->GetOrCreateChild(word, word);
		}
		
		assert(currNode != NULL);
	}
	
	currNode->SetEntropy(entropy);
	return *currNode;
}

void PhraseDictionarySourceLabel::AddEquivPhrase(TargetPhraseCollection	&targetPhraseColl, TargetPhrase *targetPhrase)
{
	targetPhraseColl.Add(targetPhrase);
}


const TargetPhraseCollection *PhraseDictionarySourceLabel::GetTargetPhraseCollection(const Phrase &source) const
{ // exactly like CreateTargetPhraseCollection, but don't create
	assert(false);
	return NULL;
	/*
	const size_t size = source.GetSize();

	const PhraseDictionaryNodeSourceLabel *currNode = &m_collection;
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		const Word& word = source.GetWord(pos);
		currNode = currNode->GetChild(word);
		if (currNode == NULL)
			return NULL;
	}

	return currNode->GetTargetPhraseCollection();
	*/
}

void PhraseDictionarySourceLabel::InitializeForInput(const InputType& input)
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

PhraseDictionarySourceLabel::~PhraseDictionarySourceLabel()
{
	CleanUp();
}

void PhraseDictionarySourceLabel::SetWeightTransModel(const vector<float> &weightT)
{
	PhraseDictionaryNodeSourceLabel::iterator iterDict;
	for (iterDict = m_collection.begin() ; iterDict != m_collection.end() ; ++iterDict)
	{
		PhraseDictionaryNodeSourceLabel::InnerNodeMap &innerNode = iterDict->second;
		PhraseDictionaryNodeSourceLabel::InnerNodeMap::iterator iterInner;
		for (iterInner = innerNode.begin() ; iterInner != innerNode.end() ; ++iterInner)
		{
			// recursively set weights in nodes
			PhraseDictionaryNodeSourceLabel &node = iterInner->second;
			node.SetWeightTransModel(this, weightT);
		}
	}
}


void PhraseDictionarySourceLabel::CleanUp()
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

TO_STRING_BODY(PhraseDictionarySourceLabel);

// friend
ostream& operator<<(ostream& out, const PhraseDictionarySourceLabel& phraseDict)
{
	const PhraseDictionaryNodeSourceLabel &coll = phraseDict.m_collection;
	PhraseDictionaryNodeSourceLabel::const_iterator iter;
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		const Word &word = (*iter).first;
		out << word;
	}
	return out;
}


}

