/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <dlfcn.h>
#include <string.h>
#include <jni.h>
#include <cerrno>
#include <cstddef>
#include <vector>
#include <cassert>
#include <string>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include "MosesUIInterface.h"
#include "moses/src/TypeDef.h"
#include "moses/src/Util.h"
#include "moses/src/Timer.h"
#include "moses/src/StaticData.h"
#include "moses/src/Manager.h"
#include "moses/src/UserMessage.h"
#include "tokenizer/Tokenizer.h"

using namespace std;
using namespace Moses;

extern "C" {
JNIEXPORT jint JNICALL Java_mosesui_app_JNIWrapper_loadModel(JNIEnv * env, jobject obj
						, jstring appPath
						, jstring iniPath
						, jstring source
						, jstring target
						, jstring description);
JNIEXPORT jstring JNICALL Java_mosesui_app_JNIWrapper_translateSentence(JNIEnv * env, jobject obj
						, jstring source);
};

// IMPLEMENTATION

jint Java_mosesui_app_JNIWrapper_loadModel(JNIEnv * env, jobject obj
						, jstring appPath
						, jstring iniPath
						, jstring source
						, jstring target
						, jstring description)
{
	const char *appPathChar = env->GetStringUTFChars(appPath, NULL);
    assert(NULL != appPathChar);

    const char *iniPathChar = env->GetStringUTFChars(iniPath, NULL);
    assert(NULL != iniPathChar);

    chdir(appPathChar);

	// load data structures
	UserMessage::SetOutput(false, true);
	Parameter *param = new Parameter();
	if (!param->LoadParam(iniPathChar))
	{
		delete param;
		return 1;
	}

	StaticData::Reset();
	const StaticData &staticData = StaticData::Instance();
	if (!staticData.LoadDataStatic(param))
		return 2;
	const std::vector<std::string> &descr = staticData.GetDescription();

	/*
	if (descr.size() < 3)
	{
		strcat(sourceChar, "unknown source language");
		strcat(targetChar, "unknown target language");
		strcat(descriptionChar, "description goes here");
	}
	else
	{
		strcat(sourceChar, descr[0].c_str());
		strcat(targetChar, descr[1].c_str());
		strcat(descriptionChar, descr[2].c_str());

		string sourceLanguageCode = staticData.GetDescription()[0];
	}
	*/

	return 0;
}

jstring Java_mosesui_app_JNIWrapper_translateSentence(JNIEnv * env, jobject obj
						, jstring source)
{
    // convert Java string to UTF-8
	const char *sourceChar = env->GetStringUTFChars(source, NULL);
    assert(NULL != sourceChar);

	const StaticData &staticData = StaticData::Instance();
	std::vector<FactorType> factorOrder;
	factorOrder.push_back(0);

	string inputStr = StringToLower(sourceChar);

	vector<string> sentences = SegmentSentenceAndWord(inputStr);

	char output[1000];
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

#ifdef LM_INTERNAL
	strcat(output, "INTERNAL ");
#endif
#ifdef LM_IRST
	strcat(output, "IRSTLM ");
#endif

    return env->NewStringUTF(output);

}

string StringToLower(string strToConvert)
{//change each element of the string to lower case
	for(unsigned int i=0;i<strToConvert.length();i++)
	{
		strToConvert[i] = tolower(strToConvert[i]);
	}
	return strToConvert;//return the converted string
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


