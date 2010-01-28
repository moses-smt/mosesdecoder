#include "PhraseDictionaryDynSuffixArray.h"
#include "DynSAInclude/utils.h"
#include "FactorCollection.h"

namespace Moses {
  PhraseDictionaryDynSuffixArray::PhraseDictionaryDynSuffixArray(size_t numScoreComponent):
    PhraseDictionary(numScoreComponent) { 
    srcSA_ = 0; 
    trgSA_ = 0;
    srcCrp_ = new vector<wordID_t>();
    trgCrp_ = new vector<wordID_t>();
    vocab_ = new Vocab(false);
  }
  PhraseDictionaryDynSuffixArray::~PhraseDictionaryDynSuffixArray(){
    delete srcSA_;
    delete trgSA_;
    delete vocab_;
    delete srcCrp_;
    delete trgCrp_;
  }
bool PhraseDictionaryDynSuffixArray::Load(string source, string target, string alignments
																					, const std::vector<float> &weight
																					, size_t tableLimit
																					, const LMList &languageModels
																					, float weightWP) 
{

	m_weight = weight;
	m_tableLimit = tableLimit;
	m_languageModels = &languageModels;
	m_weightWP = weightWP;

	loadCorpus(new FileHandler(source), *srcCrp_, srcSntBreaks_);
  loadCorpus(new FileHandler(target), *trgCrp_, trgSntBreaks_);
  //assert(srcSntBreaks_.size() == trgSntBreaks_.size());
  std::cerr << "Vocab: " << std::endl;
  vocab_->printVocab();
  // build suffix arrays and auxilliary arrays
  srcSA_ = new DynSuffixArray(srcCrp_); 
  if(!srcSA_) return false;
  //trgSA_ = new DynSuffixArray(trgCrp_); 
  //if(!trgSA_) return false;
  loadAlignments(new FileHandler(alignments));
  return true;
}
int PhraseDictionaryDynSuffixArray::loadAlignments(FileHandler* align) {
  alignments_ = new vector<vector<pair<short, short> > >();
  string line;
  vector<int> vtmp;
  while(getline(*align, line)) {
    vector<pair<short, short> > sntAlg;
    Utils::splitToInt(line, vtmp, "- ");    
    assert(vtmp.size() % 2 == 0);
    for(int i=0; i < vtmp.size(); i+=2) {
      pair<short, short> ap = std::make_pair(vtmp[i], vtmp[i+1]);
      sntAlg.push_back(ap);
    }
    alignments_->push_back(sntAlg);
    for(algItr_ = alignments_->begin(); algItr_ != alignments_->end(); ++algItr_) 
      for(sntAlgItr_ = algItr_->begin(); sntAlgItr_ != algItr_->end(); ++sntAlgItr_) {
        std::cerr << sntAlgItr_->first << "-" << sntAlgItr_->second << " ";
      }
    std::cerr  << std::endl;
  }
	
	
  return alignments_->size();
}
	
void PhraseDictionaryDynSuffixArray::LoadVocabLookup()
{
	FactorCollection &factorCollection = FactorCollection::Instance();
	
	Vocab::Word2Id::const_iterator iter;
	for (iter = vocab_->vocabStart(); iter != vocab_->vocabEnd(); ++iter)
	{
		const word_t &str = iter->first;
		wordID_t arrayId = iter->second;
		const Factor *factor = factorCollection.AddFactor(Input, 0, str, false);
		vocabLookup_[factor] = arrayId;
		vocabLookupRev_[arrayId] = factor;
	}
			
}
	
float PhraseDictionaryDynSuffixArray::getPhraseProb(vector<unsigned>* phrase) {
  srcSA_->countPhrase(phrase);
  return 0;
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
  corpus->seekg(0);
  while(getline(*corpus, line)) {
    sntArray.push_back(sntIdx);
    std::istringstream ss(line.c_str());
    while(ss >> word) {
      ++sntIdx;
      cArray.push_back(vocab_->getWordID(word));
    }          
  }
  cArray.push_back(Vocab::kOOVWordID);  // signify end of corpus for ssarray
  return cArray.size();
}
	
const TargetPhraseCollection *PhraseDictionaryDynSuffixArray::GetTargetPhraseCollection(const Phrase& src) const 
{
	const TargetPhraseCollection *ret = new const TargetPhraseCollection();

	size_t phraseSize = src.GetSize();
	for (size_t pos = 0; pos < phraseSize; ++pos)
	{
		const Word &word = src.GetWord(pos);
		const Factor *factor = word.GetFactor(0);
		
		std::map<const Factor *, wordID_t>::const_iterator iterLookup;
		iterLookup = vocabLookup_.find(factor);
		
		if (iterLookup == vocabLookup_.end())
		{ // vocab doesn't exist -> phrase doesn't exist

		}
		else
		{
			wordID_t arrayId = iterLookup->second;
	
		}
		
	}
	
	return ret;
} 

}// end namepsace
