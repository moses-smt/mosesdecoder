#ifndef MOSES_DICTSFXARY_H
#define MOSES_DICTSFXARY_H
 
#include "PhraseDictionary.h"
#include "DynSuffixArray.h" 
#include "DynSAInclude/vocab.h"
#include "DynSAInclude/file.h"
#include "DynSAInclude/types.h"

namespace Moses {

class PhraseDictionaryDynSuffixArray: public PhraseDictionary {
public: 
  PhraseDictionaryDynSuffixArray(size_t numScoreComponent);
  ~PhraseDictionaryDynSuffixArray();
  bool Load(string source, string target, string alignments);
  // functions below required by parent class
  PhraseTableImplementation GetPhraseTableImplementation() const { return dynSuffixArray; }
  void SetWeightTransModel(const std::vector<float, std::allocator<float> >&);
  const TargetPhraseCollection* GetTargetPhraseCollection(const Phrase& src) const {return new const TargetPhraseCollection();} 
  void InitializeForInput(const InputType& i);
  const ChartRuleCollection* GetChartRuleCollection(InputType const& src, WordsRange const& range,
    bool adhereTableLimit,const CellCollection &cellColl) const {}
  void CleanUp();
private:
  DynSuffixArray* srcSA_;
  DynSuffixArray* trgSA_;
  vector<wordID_t>* srcCrp_, srcSntBreaks_;
  vector<wordID_t>* trgCrp_, trgSntBreaks_;
  Vocab* vocab_;
  int loadCorpus(FileHandler* corpus, vector<wordID_t>&, vector<wordID_t>&);
};

} // end namespace
#endif
