// $Id$
#pragma once

#include "TranslationOptionCollection.h"

class Sentence;
class LMList;

class TranslationOptionCollectionText : public TranslationOptionCollection {
protected:
	const LMList *m_allLM;
	std::list<const PhraseDictionaryBase*>			m_allPhraseDictionary;
	std::list<const GenerationDictionary*>	m_allGenerationDictionary;
	
	void ProcessInitialTranslation(const DecodeStep &decodeStep
															, FactorCollection &factorCollection
															, float weightWordPenalty
															, int dropUnknown
															, size_t verboseLevel
															, PartialTranslOptColl &outputPartialTranslOptColl);
	void ProcessUnknownWord(		size_t sourcePos
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);
	void ProcessTranslation(		const TranslationOption &inputPartialTranslOpt
															, const DecodeStep &decodeStep
															, PartialTranslOptColl &outputPartialTranslOptColl
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);
	void ProcessGeneration(			const TranslationOption &inputPartialTranslOpt
															, const DecodeStep &decodeStep
															, PartialTranslOptColl &outputPartialTranslOptColl
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);
	
 public:
	TranslationOptionCollectionText(Sentence const& inputSentence);
	
	void CreateTranslationOptions(const std::list < DecodeStep > &decodeStepList
																				, const LMList &allLM
																				, FactorCollection &factorCollection
																				, float weightWordPenalty
																				, bool dropUnknown
																				, size_t verboseLevel);	
};

