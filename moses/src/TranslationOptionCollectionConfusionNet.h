// $Id: TranslationOptionCollectionConfusionNet.h 1956 2008-12-13 12:08:55Z phkoehn $
#pragma once

#include "TranslationOptionCollection.h"

namespace Moses
{

class ConfusionNet;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
 public:
	TranslationOptionCollectionConfusionNet(const ConfusionNet &source, size_t maxNoTransOptPerCoverage, float translationOptionThreshold);

	void ProcessUnknownWord(		size_t sourcePos);

};

}
