// $Id: TranslationOptionCollectionConfusionNet.h 3394 2010-08-10 13:12:00Z bhaddow $
#ifndef moses_TranslationOptionCollectionConfusionNet_h
#define moses_TranslationOptionCollectionConfusionNet_h

#include "TranslationOptionCollection.h"

namespace Moses
{

class ConfusionNet;
class TranslationSystem;

class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection {
 public:
   TranslationOptionCollectionConfusionNet(const TranslationSystem* system, const ConfusionNet &source, size_t maxNoTransOptPerCoverage, float translationOptionThreshold);

	void ProcessUnknownWord(size_t sourcePos);

};

}
#endif
