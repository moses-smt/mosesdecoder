// $Id: IOCommandLine.h 906 2006-10-21 16:31:45Z hieuhoang1972 $

/***********************************************************************
 Copyright (c) 2006 Hieu Hoang
 All rights reserved.
 ***********************************************************************/

#include "CFunctions.h"
#include "TypeDef.h"
#include "Util.h"
#include "Timer.h"
#include "StaticData.h"
#include "Manager.h"
#include "UserMessage.h"
#include "Tokenizer.h"

using namespace std;
using namespace Moses;

// helper function
void HypoToString(const Hypothesis &hypo, char *output);

int LoadModel(const char *appDir, const char *iniPath, char *source, char *target, char *description)
{
	chdir(appDir);
	
	// load data structures
	UserMessage::SetOutput(false, true);
	Parameter *param = new Parameter();
	if (!param->LoadParam(iniPath))
	{
		delete param;
		return 1;
	}
	
	const StaticData &staticData = StaticData::Instance();
	if (!staticData.LoadDataStatic(param))
		return 2;
	const std::vector<std::string> &descr = staticData.GetDescription();

	if (descr.size() < 3)
	{
		strcat(source, "unknown source language");
		strcat(target, "unknwon target language");
		strcat(description, "description goes here");		
	}
	else	
	{
		strcat(source, descr[0].c_str());
		strcat(target, descr[1].c_str());
		strcat(description, descr[2].c_str());
		
		string sourceLanguageCode = staticData.GetDescription()[0];
	}
	
	return 0;
}

string StringToLower(string strToConvert)
{//change each element of the string to lower case
	for(unsigned int i=0;i<strToConvert.length();i++)
	{
		strToConvert[i] = tolower(strToConvert[i]);
	}
	return strToConvert;//return the converted string
} 


int TranslateSentence(const char *rawInput, char *output)
{
	const StaticData &staticData = StaticData::Instance();
	
	std::vector<FactorType> factorOrder;
	factorOrder.push_back(0);
	
	string inputStr = StringToLower(rawInput);
	
	vector<string> sentences = SegmentSentenceAndWord(inputStr);
	
	strcpy(output, "");
	vector<string>::iterator iterSentences;
	for (iterSentences = sentences.begin() ; iterSentences != sentences.end() ; ++iterSentences)
	{
		const string &input = *iterSentences;
		
		Sentence *sentence = new Sentence(Input);
		sentence->CreateFromString(factorOrder, input, "|");
		
		const TranslationSystem& system = staticData.GetTranslationSystem(TranslationSystem::DEFAULT);

		Manager manager(*sentence, staticData.GetSearchAlgorithm(), &system);
		manager.ProcessSentence();
		const Hypothesis *hypo = manager.GetBestHypothesis();
		
		if (hypo != NULL)
		{
			HypoToString(*hypo, output);
		}
		
		
		delete sentence;
	}
	
  return strlen(output);
}

void HypoToString(const Hypothesis &hypo, char *output)
{
	const StaticData &staticData = StaticData::Instance();
	const std::vector<FactorType> outFactor = staticData.GetOutputFactorOrder();
	assert(outFactor.size() == 1);
	
	stringstream strme;
	size_t hypoSize = hypo.GetSize();
	
	for (size_t pos = 0 ; pos < hypoSize ; ++pos)
	{
		const Word &word = hypo.GetWord(pos);
		strme << *word[outFactor[0]] << " ";
	}
	
	string str = strme.str();
	strcat(output, str.c_str());
}

void GetErrorMessages(char *msg)
{
	strcpy(msg, UserMessage::GetQueue().c_str());
}

std::vector<std::string> SegmentSentenceAndWord(const std::string &sentence)
{
	std::vector<std::string> ret;	
	Tokenizer tokenizer;
	
	ret = tokenizer.Tokenize(sentence, SentenceInput);
	
	return ret;
}


