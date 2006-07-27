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
#include <sys/stat.h>
#include "boost/filesystem/operations.hpp" // includes boost/filesystem/path.hpp
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "Word.h"
#include "Util.h"
#include "InputFileStream.h"
#include "StaticData.h"
#include "Input.h"
#include "WordsRange.h"

using namespace std;

PhraseDictionaryBase::PhraseDictionaryBase(size_t noScoreComponent)
	: Dictionary(noScoreComponent),m_maxTargetPhrase(0)
{
}

PhraseDictionaryBase::~PhraseDictionaryBase() {}
	
const TargetPhraseCollection *PhraseDictionaryBase::
GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const 
{
	return GetTargetPhraseCollection(src.GetSubString(range));
}

const std::string PhraseDictionaryBase::GetScoreProducerDescription() const
{
	return "Translation score, file=" + m_filename;
}

unsigned int PhraseDictionaryBase::GetNumScoreComponents() const
{
	return this->GetNoScoreComponents();
}

void PhraseDictionary::Load(const std::vector<FactorType> &input
																			, const std::vector<FactorType> &output
																			, FactorCollection &factorCollection
																			, const string &filePath
																			, const string &hashFilePath
																			, const vector<float> &weight
																			, size_t maxTargetPhrase
																			, bool filter
																			, const list< Phrase > &inputPhraseList
																			, const LMList &languageModels
																			, float weightWP
																			, const StaticData& staticData)
{
	m_maxTargetPhrase = maxTargetPhrase;
	m_filename = filePath;

	//factors	
	m_factorsUsed[Input]	= new FactorTypeSet(input);
	m_factorsUsed[Output]	= new FactorTypeSet(output);

	// data from file
	InputFileStream inFile(filePath);

	// create hash file if necessary
	ofstream tempFile;
	string tempFilePath;
	if (filter)
	{
		CreateTempFile(tempFile, tempFilePath);
		TRACE_ERR(filePath << " -> " << tempFilePath << " -> " << hashFilePath << endl);
	}

	vector< vector<string> >	phraseVector;
	string line, prevSourcePhrase = "";
	bool addPhrase = !filter;
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

		if (!filter)
		{
			if (tokens[0] != prevSourcePhrase)
				phraseVector = Phrase::Parse(tokens[0]);
		}
		else if (tokens[0] == prevSourcePhrase)
		{ // same source phrase as prev line.
		}
		else
		{
			phraseVector = Phrase::Parse(tokens[0]);
			prevSourcePhrase = tokens[0];

			addPhrase = Contains(phraseVector, inputPhraseList, input);
		}

		if (addPhrase)
		{
			vector<float> scoreVector = Tokenize<float>(tokens[2]);
			if (scoreVector.size() != m_noScoreComponent) {
				TRACE_ERR("Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_noScoreComponent<<") of score components on line " << line_num);
        abort();
			}
//			assert(scoreVector.size() == m_noScoreComponent);
			
			// source
			Phrase sourcePhrase(Input);
			sourcePhrase.CreateFromString( input, phraseVector, factorCollection);
			//target
			TargetPhrase targetPhrase(Output, this);
			targetPhrase.CreateFromString( output, tokens[1], factorCollection);

			// component score, for n-best output
			targetPhrase.SetScore(scoreVector, weight, languageModels, weightWP);

			AddEquivPhrase(sourcePhrase, targetPhrase);

			// add to hash file
			if (filter)
				tempFile << line << endl;
		}
		count++;
	}

	// move temp file to hash file
	if (filter)
	{
		tempFile.close();
		using namespace boost::filesystem;
		if (!exists(path(hashFilePath, native)))
		{
			try 
			{
				rename( path(tempFilePath, native) , path(hashFilePath, native) );
			}
			catch (...)
			{ // copy instead
				copy_file(path(tempFilePath, native) , path(hashFilePath, native) );
				remove(tempFilePath);
			}
		}
#ifndef _WIN32
		// change permission to let everyone use cached file
		chmod(hashFilePath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif
		TRACE_ERR("Size " << count << " -> " << GetSize() << endl);
	}
	else
	{
		TRACE_ERR("Size " << GetSize() << endl);
	}
}

void PhraseDictionary::AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)
{
	if (m_maxTargetPhrase == 0)
	{	// don't need keep list sorted
		m_collection[source].push_back(targetPhrase);
	}
	else
	{	// must keep list in sorted order
		TargetPhraseCollection &phraseColl = m_collection[source];
		TargetPhraseCollection::iterator iter;
		for (iter = phraseColl.begin() ; iter != phraseColl.end() ; ++iter)
		{
			TargetPhrase &insertPhrase = *iter;
			if (targetPhrase.GetFutureScore() < insertPhrase.GetFutureScore())
			{
				break;
			}
		}
		phraseColl.insert(iter, targetPhrase);

		// get rid of least probable phrase if we have enough
		if (phraseColl.size() > m_maxTargetPhrase)
		{
			phraseColl.erase(phraseColl.begin());
		}
	}
}

const TargetPhraseCollection *PhraseDictionary::GetTargetPhraseCollection(const Phrase &source) const
{
	std::map<Phrase , TargetPhraseCollection >::const_iterator iter = m_collection.find(source);
	if (iter == m_collection.end())
	{ // can't find source phrase
		return NULL;
	}
	
	return &iter->second;
}

PhraseDictionary::~PhraseDictionary()
{
	for (size_t i = 0 ; i < m_factorsUsed.size() ; i++)
	{
		delete m_factorsUsed[i];
	}
}

void PhraseDictionary::SetWeightTransModel(const vector<float> &weightT)
{
	std::map<Phrase , TargetPhraseCollection >::iterator iterDict;
	for (iterDict = m_collection.begin() ; iterDict != m_collection.end() ; ++iterDict)
	{
		TargetPhraseCollection &targetPhraseCollection = iterDict->second;
		
		TargetPhraseCollection::iterator targetPhraseIter;
		for (targetPhraseIter = targetPhraseCollection.begin();
					targetPhraseIter != targetPhraseCollection.end();
					++targetPhraseIter)
		{
			targetPhraseIter->SetWeights(weightT);
		}
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

TO_STRING_BODY(PhraseDictionary);

// friend
ostream& operator<<(ostream& out, const PhraseDictionary& phraseDict)
{
	const map<Phrase , TargetPhraseCollection > &coll = phraseDict.m_collection;
	map<Phrase , TargetPhraseCollection >::const_iterator iter;	
	for (iter = coll.begin() ; iter != coll.end() ; ++iter)
	{
		const Phrase &sourcePhrase = (*iter).first;
		out << sourcePhrase;
	}
	return out;
}

