// $Id$

#ifndef moses_PhraseDictionaryTreeAdaptor_h
#define moses_PhraseDictionaryTreeAdaptor_h

#include <vector>
#include "util/check.hh"
#include "moses/TypeDef.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/TranslationModel/PhraseDictionary.h"

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

  boost::thread_specific_ptr<PDTAimp> m_implementation;

  friend class PDTAimp;
  PhraseDictionaryTreeAdaptor();
  PhraseDictionaryTreeAdaptor(const PhraseDictionaryTreeAdaptor&);
  void operator=(const PhraseDictionaryTreeAdaptor&);

  PDTAimp& GetImplementation();
  const PDTAimp& GetImplementation() const;

public:
  PhraseDictionaryTreeAdaptor(const std::string &line);
  virtual ~PhraseDictionaryTreeAdaptor();

  // enable/disable caching
  // you enable caching if you request the target candidates for a source phrase multiple times
  // if you do caching somewhere else, disable it
  // good settings for current Moses: disable for first factor, enable for other factors
  // default: enable

  void EnableCache();
  void DisableCache();

  // initialize ...
  bool InitDictionary();

  // get translation candidates for a given source phrase
  // returns null pointer if nothing found
  TargetPhraseCollection const* GetTargetPhraseCollection(Phrase const &src) const;
  TargetPhraseCollection const* GetTargetPhraseCollection(InputType const& src,WordsRange const & srcRange) const;

  void InitializeForInput(InputType const& source);
  void CleanUpAfterSentenceProcessing(InputType const& source);

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &) {
    CHECK(false);
    return 0;
  }
};

}
#endif
