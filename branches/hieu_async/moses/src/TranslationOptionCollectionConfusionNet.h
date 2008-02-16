// $Id: TranslationOptionCollectionConfusionNet.h 147 2007-10-14 21:36:11Z hieu $
#pragma once

#include "TranslationOptionCollection.h"

class ConfusionNet;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
 public:
	TranslationOptionCollectionConfusionNet(const ConfusionNet &source);

	void ProcessUnknownWord(size_t decodeStepId, size_t sourcePos);

};
