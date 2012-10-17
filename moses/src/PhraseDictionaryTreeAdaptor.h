// $Id$

#ifndef moses_PhraseDictionaryTreeAdaptor_h
#define moses_PhraseDictionaryTreeAdaptor_h

#include <vector>
#include "util/check.hh"
#include "TypeDef.h"
#include "PhraseDictionaryMemory.h"
#include "TargetPhraseCollection.h"

namespace Moses
{

class Phrase;
class PDTAimp;
class WordsRange;
class InputType;

/*** Implementation of a phrase table in a trie that is binarized and
 * stored on disk. Wrapper around PDTAimp class
 */
class PhraseDictionaryTreeAdaptor : public PhraseDictionary
{
  typedef PhraseDictionary MyBase;
  PDTAimp *imp;
  friend class PDTAimp;
  PhraseDictionaryTreeAdaptor();
  PhraseDictionaryTreeAdaptor(const PhraseDictionaryTreeAdaptor&);
  void operator=(const PhraseDictionaryTreeAdaptor&);

public:
  PhraseDictionaryTreeAdaptor(size_t numScoreComponent, unsigned numInputScores, const PhraseDictionaryFeature* feature);
  virtual ~PhraseDictionaryTreeAdaptor();

  // enable/disable caching
  // you enable caching if you request the target candidates for a source phrase multiple times
  // if you do caching somewhere else, disable it
  // good settings for current Moses: disable for first factor, enable for other factors
  // default: enable

  void EnableCache();
  void DisableCache();

  // initialize ...
  bool Load(const std::vector<FactorType> &input
            , const std::vector<FactorType> &output
            , const std::string &filePath
            , const std::vector<float> &weight
            , size_t tableLimit
            , const LMList &languageModels
            , float weightWP);

  // get translation candidates for a given source phrase
  // returns null pointer if nothing found
  TargetPhraseCollection const* GetTargetPhraseCollection(Phrase const &src) const;
  TargetPhraseCollection const* GetTargetPhraseCollection(InputType const& src,WordsRange const & srcRange) const;

  std::string GetScoreProducerDescription(unsigned idx=0) const;
  std::string GetScoreProducerWeightShortName(unsigned idx=0) const;

  size_t GetNumInputScores() const;
  virtual void InitializeForInput(InputType const& source);

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &) {
    CHECK(false);
    return 0;
  }
};

}
#endif
