// $Id$

#include <cassert>
#include <iostream>
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeStep.h"
#include "FactorCollection.h"
#include "moses/FF/InputFeature.h"

using namespace std;

namespace Moses
{

/** constructor; just initialize the base class */
TranslationOptionCollectionConfusionNet::TranslationOptionCollectionConfusionNet(
  const ConfusionNet &input
  , size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : TranslationOptionCollection(input, maxNoTransOptPerCoverage, translationOptionThreshold)
{
  const StaticData &staticData = StaticData::Instance();
  const InputFeature *inputFeature = staticData.GetInputFeature();
  CHECK(inputFeature);

  size_t size = input.GetSize();
  for (size_t startPos = 0; startPos < size; ++startPos) {
    // create matrix
    std::vector<std::vector<SourcePath> > vec;
    m_collection.push_back( vec );
    size_t maxSize = size - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = std::min(maxSize, maxSizePhrase);

    for (size_t endPos = 0 ; endPos < maxSize ; ++endPos) {
      std::vector<SourcePath> vec;
      m_collection[startPos].push_back( vec );
    }


    // cut up confusion network into substrings
    // start with 1-word phrases
    std::vector<SourcePath> &subphrases = GetPhrases(startPos, startPos);
    assert(subphrases.size() == 0);

    const ConfusionNet::Column &col = input.GetColumn(startPos);
    ConfusionNet::Column::const_iterator iter;
    for (iter = col.begin(); iter != col.end(); ++iter) {
      subphrases.push_back(SourcePath());
      SourcePath &sourcePath = subphrases.back();

      const std::pair<Word,std::vector<float> > &inputNode = *iter;

      //cerr << "word=" << inputNode.first << " scores=" << inputNode.second.size() << endl;
      sourcePath.first.AddWord(inputNode.first);
      sourcePath.second.PlusEquals(inputFeature, inputNode.second);

    }
  }

  for (size_t startPos = 0; startPos < input.GetSize(); ++startPos) {
    for (size_t endPos = startPos; endPos < input.GetSize(); ++endPos) {

    }
  }


}

/* forcibly create translation option for a particular source word.
	* call the base class' ProcessOneUnknownWord() for each possible word in the confusion network
	* at a particular source position
*/
void TranslationOptionCollectionConfusionNet::ProcessUnknownWord(size_t sourcePos)
{
  ConfusionNet const& source=dynamic_cast<ConfusionNet const&>(m_source);

  ConfusionNet::Column const& coll=source.GetColumn(sourcePos);
  size_t j=0;
  for(ConfusionNet::Column::const_iterator i=coll.begin(); i!=coll.end(); ++i) {
    ProcessOneUnknownWord(i->first ,sourcePos, source.GetColumnIncrement(sourcePos, j++),&(i->second));
  }

}

const std::vector<TranslationOptionCollectionConfusionNet::SourcePath> &TranslationOptionCollectionConfusionNet::GetPhrases(size_t startPos, size_t endPos) const
{
  size_t maxSize = endPos - startPos;
  //size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
  //maxSize = std::min(maxSize, maxSizePhrase);

  CHECK(maxSize < m_collection[startPos].size());
  return m_collection[startPos][maxSize];

}

std::vector<TranslationOptionCollectionConfusionNet::SourcePath> &TranslationOptionCollectionConfusionNet::GetPhrases(size_t startPos, size_t endPos)
{
  size_t maxSize = endPos - startPos;
  //size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
  //maxSize = std::min(maxSize, maxSizePhrase);

  CHECK(maxSize < m_collection[startPos].size());
  return m_collection[startPos][maxSize];

}

} // namespace


