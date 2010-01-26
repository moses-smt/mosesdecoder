#include "PhraseDictionaryDynSuffixArray.h"
#include "DynSAInclude/utils.h"

namespace Moses {
  PhraseDictionaryDynSuffixArray::PhraseDictionaryDynSuffixArray(size_t numScoreComponent):
    PhraseDictionary(numScoreComponent) { 
    srcSA_ = new DynSuffixArray(); 
    trgSA_ = new DynSuffixArray(); 
    srcCrp_ = new vector<wordID_t>();
    trgCrp_ = new vector<wordID_t>();
    vocab_ = new Vocab();
  }
  PhraseDictionaryDynSuffixArray::~PhraseDictionaryDynSuffixArray(){
    delete srcSA_;
    delete trgSA_;
    delete vocab_;
    delete srcCrp_;
    delete trgCrp_;
  }
bool PhraseDictionaryDynSuffixArray::Load(string source, string target, string alignments) {
  loadCorpus(new FileHandler(source), *srcCrp_, srcSntBreaks_);
  loadCorpus(new FileHandler(target), *trgCrp_, trgSntBreaks_);
  assert(srcSntBreaks_.size() == trgSntBreaks_.size());
}
void PhraseDictionaryDynSuffixArray::InitializeForInput(const InputType& input)
{
  /*assert(m_runningNodesVec.size() == 0);
  size_t sourceSize = input.GetSize();
  m_runningNodesVec.resize(sourceSize);
  for (size_t ind = 0; ind < m_runningNodesVec.size(); ++ind)
  {
    ProcessedRule *initProcessedRule = new ProcessedRule(m_collection);
    ProcessedRuleStack *processedStack = new ProcessedRuleStack(sourceSize - ind + 1);
    processedStack->Add(0, initProcessedRule); // init rule. stores the top node in tree
    m_runningNodesVec[ind] = processedStack;
  }*/
  return;
}
void PhraseDictionaryDynSuffixArray::CleanUp() {
  return;
}
void PhraseDictionaryDynSuffixArray::SetWeightTransModel(const std::vector<float, std::allocator<float> >&) {
  return;
}
int PhraseDictionaryDynSuffixArray::loadCorpus(FileHandler* corpus, vector<wordID_t>& cArray, 
    vector<wordID_t>& sntArray) {
  string line, word;
  int sntIdx(0);
  while(getline(*corpus, line)) {
    sntArray.push_back(sntIdx);
    std::istringstream ss(line.c_str());
    while(ss >> word) {
      ++sntIdx;
      cArray.push_back(vocab_->getWordID(word));
    }          
  }
  iterate(cArray, itr) cerr << *itr << endl;
  return cArray.size();
}
}// end namepsace
