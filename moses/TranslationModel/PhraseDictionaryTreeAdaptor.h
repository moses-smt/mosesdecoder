// $Id$

#ifndef moses_PhraseDictionaryTreeAdaptor_h
#define moses_PhraseDictionaryTreeAdaptor_h

#include "moses/TypeDef.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include <vector>

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#else
#include <boost/scoped_ptr.hpp>
#endif

namespace Moses
{

class Phrase;
class PDTAimp;
class Range;
class InputType;

/*** Implementation of a phrase table in a trie that is binarized and
 * stored on disk. Wrapper around PDTAimp class
 */
class PhraseDictionaryTreeAdaptor : public PhraseDictionary
{
  typedef PhraseDictionary MyBase;

#ifdef WITH_THREADS
  boost::thread_specific_ptr<PDTAimp> m_implementation;
#else
  boost::scoped_ptr<PDTAimp> m_implementation;
#endif

  friend class PDTAimp;
  PhraseDictionaryTreeAdaptor();
  PhraseDictionaryTreeAdaptor(const PhraseDictionaryTreeAdaptor&);
  void operator=(const PhraseDictionaryTreeAdaptor&);

  PDTAimp& GetImplementation();
  const PDTAimp& GetImplementation() const;

public:
  PhraseDictionaryTreeAdaptor(const std::string &line);
  virtual ~PhraseDictionaryTreeAdaptor();
  void Load(AllOptions::ptr const& opts);

  // enable/disable caching
  // you enable caching if you request the target candidates for a source phrase multiple times
  // if you do caching somewhere else, disable it
  // good settings for current Moses: disable for first factor, enable for other factors
  // default: enable

  void EnableCache();
  void DisableCache();

  // get translation candidates for a given source phrase
  // returns null pointer if nothing found
  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollectionNonCacheLEGACY(Phrase const &src) const;

  void InitializeForInput(ttasksptr const& ttask);
  void CleanUpAfterSentenceProcessing(InputType const& source);

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &,
    const ChartCellCollectionBase &,
    std::size_t) {
    UTIL_THROW(util::Exception, "SCFG decoding not supported with binary phrase table");
    return 0;
  }

  // legacy
  TargetPhraseCollectionWithSourcePhrase::shared_ptr
  GetTargetPhraseCollectionLEGACY(InputType const& src,
                                  Range const & srcRange) const;

};

}
#endif
