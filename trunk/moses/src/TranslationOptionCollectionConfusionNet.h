// $Id$
#ifndef TRANSLATIONOPTIONCOLLECTIONCONFUSIONNET_H_
#define TRANSLATIONOPTIONCOLLECTIONCONFUSIONNET_H_
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



};
#endif
