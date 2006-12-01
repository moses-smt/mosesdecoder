// $Id$
#pragma once

#include "TranslationOptionCollection.h"

class ConfusionNet;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
 public:
	TranslationOptionCollectionConfusionNet(const ConfusionNet &source, size_t maxNoTransOptPerCoverage);

	void ProcessUnknownWord(const DecodeStep *decodeStep,		size_t sourcePos
															, FactorCollection &factorCollection);

};
