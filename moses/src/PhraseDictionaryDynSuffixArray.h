#ifndef moses_PhraseDictionaryDynSuffixArray_h
#define moses_PhraseDictionaryDynSuffixArray_h

#include <map>

#include "PhraseDictionary.h"
#include "BilingualDynSuffixArray.h"

namespace Moses
{

/** Implementation of a phrase table using the biconcor suffix array.
 *  Wrapper around a BilingualDynSuffixArray object
 */
class PhraseDictionaryDynSuffixArray: public PhraseDictionary
{
public:
  PhraseDictionaryDynSuffixArray(size_t m_numScoreComponent, PhraseDictionaryFeature* feature);
  ~PhraseDictionaryDynSuffixArray();
  bool Load( const std::vector<FactorType>& m_input
             , const std::vector<FactorType>& m_output
             , std::string m_source
             , std::string m_target
             , std::string m_alignments
             , const std::vector<float> &m_weight
             , size_t m_tableLimit
             , const LMList &languageModels
             , float weightWP);
  // functions below required by base class
  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const;
  void InitializeForInput(const InputType& i);
  void CleanUp(const InputType &source);
  void insertSnt(string&, string&, string&);
  void deleteSnt(unsigned, unsigned);
  ChartRuleLookupManager *CreateRuleLookupManager(const InputType&, const ChartCellCollectionBase&);
private:
  BilingualDynSuffixArray *m_biSA;
  std::vector<float> m_weight;
  size_t m_tableLimit;
  const LMList *m_languageModels;
  float m_weightWP;


};

} // end namespace
#endif
