// $Id$
#pragma once

#include "TranslationOptionCollection.h"

class ConfusionNet;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
 public:
	TranslationOptionCollectionConfusionNet(const ConfusionNet &source);

	int HandleUnkownWord(PhraseDictionaryBase& //phraseDictionary
												,size_t //startPos
												,FactorCollection & //factorCollection
												,const LMList &//allLM
												,bool //dropUnknown
												,float //weightWordPenalty
											 ) {return 0;}

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

};
