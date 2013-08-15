// $Id$

#ifndef moses_PhraseDictionaryTreeAdaptor_h
#define moses_PhraseDictionaryTreeAdaptor_h

#include "moses/TypeDef.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/TranslationModel/PhraseDictionary.h"
#include "util/check.hh"
#include <vector>

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#include <boost/thread/shared_mutex.hpp>
#else
#include <boost/scoped_ptr.hpp>
#endif

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

  // cache
  bool m_useCache;
  mutable std::map<size_t, const TargetPhraseCollection*> m_cache;
#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::shared_mutex m_accessLock;
#endif

public:
  PhraseDictionaryTreeAdaptor(const std::string &line);
  virtual ~PhraseDictionaryTreeAdaptor();
  void Load();

  void SetParameter(const std::string& key, const std::string& value);

  // enable/disable caching
  // you enable caching if you request the target candidates for a source phrase multiple times
  // if you do caching somewhere else, disable it
  // good settings for current Moses: disable for first factor, enable for other factors
  // default: enable

  void EnableCache();
  void DisableCache();

  // get translation candidates for a given source phrase
  // returns null pointer if nothing found
  TargetPhraseCollection const* GetTargetPhraseCollection(Phrase const &src) const;

  void InitializeForInput(InputType const& source);
  void CleanUpAfterSentenceProcessing(InputType const& source);

  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &,
    const ChartCellCollectionBase &) {
    CHECK(false);
    return 0;
  }

  // legacy
  const TargetPhraseCollectionWithSourcePhrase *GetTargetPhraseCollectionLegacy(InputType const& src,WordsRange const & srcRange) const;

};

}
#endif
