// $Id: TranslationOptionCollectionConfusionNet.h 1897 2008-10-08 23:51:26Z hieuhoang1972 $
#pragma once

#include "TranslationOptionCollection.h"

namespace Moses
{

class ConfusionNet;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
 public:
	TranslationOptionCollectionConfusionNet(const ConfusionNet &source, size_t maxNoTransOptPerCoverage);

	void ProcessUnknownWord(		size_t sourcePos);

};

}
