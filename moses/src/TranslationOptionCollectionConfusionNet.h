// $Id: TranslationOptionCollectionConfusionNet.h 2939 2010-02-24 11:15:44Z jfouet $
#ifndef moses_TranslationOptionCollectionConfusionNet_h
#define moses_TranslationOptionCollectionConfusionNet_h

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
#endif
