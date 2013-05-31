#ifndef moses_PhraseDictionaryDynSuffixArray_h
#define moses_PhraseDictionaryDynSuffixArray_h

#include <map>

#include "moses/TranslationModel/PhraseDictionary.h"
#include "moses/TranslationModel/BilingualDynSuffixArray.h"

namespace Moses
{

/** Implementation of a phrase table using the biconcor suffix array.
 *  Wrapper around a BilingualDynSuffixArray object
 */
class PhraseDictionaryDynSuffixArray: public PhraseDictionary
{
public:
  PhraseDictionaryDynSuffixArray(const std::string &line);
  ~PhraseDictionaryDynSuffixArray();

  void Load();

  // functions below required by base class
  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
  void insertSnt(string&, string&, string&);
  void deleteSnt(unsigned, unsigned);
  ChartRuleLookupManager *CreateRuleLookupManager(const InputType&, const ChartCellCollectionBase&);
private:
  BilingualDynSuffixArray *m_biSA;
  std::string m_source, m_target, m_alignments;

};

} // end namespace
#endif
