// $Id$
#include "TranslationOptionCollectionText.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"
#include "WordsRange.h"
#include "LMList.h"

using namespace std;

TranslationOptionCollectionText::TranslationOptionCollectionText(Sentence const &inputSentence, size_t maxNoTransOptPerCoverage) 
	: TranslationOptionCollection(inputSentence, maxNoTransOptPerCoverage) {}

void TranslationOptionCollectionText::ProcessUnknownWord(size_t sourcePos
												, int dropUnknown
												, FactorCollection &factorCollection
												, float weightWordPenalty)
{
	const FactorArray &sourceWord = m_source.GetFactorArray(sourcePos);
	ProcessOneUnknownWord(sourceWord,sourcePos,dropUnknown,factorCollection,weightWordPenalty);
}
