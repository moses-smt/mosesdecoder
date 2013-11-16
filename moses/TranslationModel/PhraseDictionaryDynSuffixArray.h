#ifndef moses_PhraseDictionaryDynSuffixArray_h
#define moses_PhraseDictionaryDynSuffixArray_h

#include <map>

#include "moses/TypeDef.h"
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
  bool InitDictionary();
  void Load();
  // functions below required by base class
  const TargetPhraseCollection* GetTargetPhraseCollectionLEGACY(const Phrase& src) const;
  void insertSnt(string&, string&, string&);
  void deleteSnt(unsigned, unsigned);
  ChartRuleLookupManager *CreateRuleLookupManager(const ChartParser &, const ChartCellCollectionBase&);
  void SetParameter(const std::string& key, const std::string& value);
private:
  BilingualDynSuffixArray *m_biSA;
  std::string m_source, m_target, m_alignments;

  std::vector<float> m_weight;
};

} // end namespace
#endif
