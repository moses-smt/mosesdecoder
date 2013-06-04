// $Id$
#ifndef moses_TranslationOptionCollectionConfusionNet_h
#define moses_TranslationOptionCollectionConfusionNet_h

#include "TranslationOptionCollection.h"

namespace Moses
{

class ConfusionNet;

/** Holds all translation options, for all spans, of a particular confusion network input
 * Inherited from TranslationOptionCollection.
 */
class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection
{
public:
  TranslationOptionCollectionConfusionNet(const ConfusionNet &source, size_t maxNoTransOptPerCoverage, float translationOptionThreshold);

  void ProcessUnknownWord(size_t sourcePos);

};

}
#endif
