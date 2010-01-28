#ifndef MOSES_DICTSFXARY_H
#define MOSES_DICTSFXARY_H

#include "PhraseDictionary.h"
#include "DynSuffixArray.h" 
#include "DynSAInclude/vocab.h"
#include "DynSAInclude/file.h"
#include "DynSAInclude/types.h"
#include "DynSAInclude/utils.h"
namespace Moses {

class PhraseDictionaryDynSuffixArray: public PhraseDictionary {
public: 
  PhraseDictionaryDynSuffixArray(size_t numScoreComponent);
  ~PhraseDictionaryDynSuffixArray();
  bool Load(string source, string target, string alignments);
  float getPhraseProb(vector<unsigned>*);
  // functions below required by base class
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
  vector<wordID_t>* srcCrp_;
  vector<wordID_t>* trgCrp_;
  vector<unsigned> srcSntBreaks_, trgSntBreaks_;
  Vocab* vocab_;
  vector<vector<pair<short, short> > >* alignments_;
  vector<vector<pair<short, short> > >::iterator algItr_;
  vector<pair<short, short> >::iterator sntAlgItr_;
  int loadCorpus(FileHandler* corpus, vector<wordID_t>&, vector<wordID_t>&);
  int loadAlignments(FileHandler* aligs);
};

} // end namespace
#endif
