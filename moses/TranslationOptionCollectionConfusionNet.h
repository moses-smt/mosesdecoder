// $Id$
#ifndef moses_TranslationOptionCollectionConfusionNet_h
#define moses_TranslationOptionCollectionConfusionNet_h

#include "TranslationOptionCollection.h"
#include "InputPath.h"

namespace Moses
{

class ConfusionNet;

/** Holds all translation options, for all spans, of a particular confusion network input
 * Inherited from TranslationOptionCollection.
 */
class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection
{
public:
  typedef std::vector< std::vector<InputPathList> > TargetPhraseMatrix;

protected:
  bool m_useLegacy;

  TargetPhraseMatrix	m_targetPhrasesfromPt; /*< contains translation options */

  InputPathList &GetInputPathList(size_t startPos, size_t endPos);
  void CheckLegacy();
public:
  TranslationOptionCollectionConfusionNet(const ConfusionNet &source, size_t maxNoTransOptPerCoverage, float translationOptionThreshold);

  void ProcessUnknownWord(size_t sourcePos);
  void CreateTranslationOptionsForRange(const DecodeGraph &decodeStepList
                                        , size_t startPosition
                                        , size_t endPosition
                                        , bool adhereTableLimit
                                        , size_t graphInd);

protected:

};

}
#endif

