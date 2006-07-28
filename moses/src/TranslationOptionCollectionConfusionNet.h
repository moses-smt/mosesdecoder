// $Id$
#pragma once

#include "TranslationOptionCollection.h"

class ConfusionNet;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
 public:
	TranslationOptionCollectionConfusionNet(const ConfusionNet &source, size_t maxNoTransOptPerCoverage);

	void ProcessUnknownWord(		size_t sourcePos
															, int dropUnknown
															, FactorCollection &factorCollection
															, float weightWordPenalty);

};
