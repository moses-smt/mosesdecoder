// $Id$
#ifndef moses_TranslationOptionCollectionConfusionNet_h
#define moses_TranslationOptionCollectionConfusionNet_h

#include "TranslationOptionCollection.h"
#include "ConfusionNet.h"

namespace Moses
{
class InputFeature;

/** Holds all translation options, for all spans, of a particular confusion network input
 * Inherited from TranslationOptionCollection.
 */
class TranslationOptionCollectionConfusionNet : public TranslationOptionCollection
{
public:
  typedef std::pair<Phrase, ScoreComponentCollection> SourcePath;

  TranslationOptionCollectionConfusionNet(const ConfusionNet &source, size_t maxNoTransOptPerCoverage, float translationOptionThreshold);

  void ProcessUnknownWord(size_t sourcePos);

  const std::vector<SourcePath> &GetPhrases(size_t startPos, size_t endPos) const;
  std::vector<SourcePath> &GetPhrases(size_t startPos, size_t endPos);
protected:
  std::vector<std::vector<std::vector<SourcePath> > > m_collection;

  void CreateSubPhrases(std::vector<SourcePath> &newSubphrases
                        , const std::vector<SourcePath> &prevSubphrases
                        , const ConfusionNet::Column &col
                        , const InputFeature &inputFeature);

  void CreateTranslationOptionsForRange(const DecodeGraph &decodeStepList
        , size_t startPosition
        , size_t endPosition
        , bool adhereTableLimit
        , size_t graphInd);
};

}
#endif
