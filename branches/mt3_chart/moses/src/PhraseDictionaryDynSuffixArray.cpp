#include "PhraseDictionaryDynSuffixArray.h"
#include "DynSAInclude/utils.h"
#include "FactorCollection.h"
#include "StaticData.h"
#include "TargetPhrase.h"

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

	InputFileStream sourceStrme(source);
	InputFileStream targetStrme(target);
	
	loadCorpus(sourceStrme, *srcCrp_, srcSntBreaks_);
  loadCorpus(targetStrme, *trgCrp_, trgSntBreaks_);
  assert(srcSntBreaks_.size() == trgSntBreaks_.size());
  std::cerr << "Vocab: " << std::endl;
  vocab_->printVocab();
	LoadVocabLookup();

  // build suffix arrays and auxilliary arrays
  srcSA_ = new DynSuffixArray(srcCrp_); 
  if(!srcSA_) return false;
  trgSA_ = new DynSuffixArray(trgCrp_); 
  if(!trgSA_) return false;
	
	InputFileStream alignStrme(alignments);
  loadAlignments(alignStrme);
	
  return true;
}
int PhraseDictionaryDynSuffixArray::loadAlignments(InputFileStream& align) {
  // stores the alignments in the format used by SentenceAlignment.Extract()
  string line;
  vector<int> vtmp;
  int sntIndex(0);
	
  while(getline(align, line)) {
    Utils::splitToInt(line, vtmp, "- ");
    assert(vtmp.size() % 2 == 0);
		
		int sourceSize = GetSourceSentenceSize(sntIndex);
		int targetSize = GetTargetSentenceSize(sntIndex);

    SentenceAlignment curSnt(sntIndex, sourceSize, targetSize); // initialize empty sentence 
    for(int i=0; i < vtmp.size(); i+=2) {
			int sourcePos = vtmp[i];
			int targetPos = vtmp[i+1];
      curSnt.alignedCountTrg[targetPos]++; // cnt of trg nodes each src node is attached to  
      curSnt.alignedSrc[sourcePos].push_back(targetPos);  // list of source nodes each target node connects with
    }
    curSnt.srcSnt = srcCrp_ + sntIndex;  // point source and target sentence
    curSnt.trgSnt = trgCrp_ + sntIndex; 
    alignments_.push_back(curSnt);
		
		sntIndex++;
  }
  return alignments_.size();
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
float PhraseDictionaryDynSuffixArray::MLEProb(int denom, vector<PhrasePair>& phrasepairs) const {
}
int PhraseDictionaryDynSuffixArray::loadCorpus(InputFileStream& corpus, vector<wordID_t>& cArray, 
    vector<wordID_t>& sntArray) {
  string line, word;
  int sntIdx(0);
  corpus.seekg(0);
  while(getline(corpus, line)) {
    sntArray.push_back(sntIdx);
    std::istringstream ss(line.c_str());
    while(ss >> word) {
      ++sntIdx;
      cArray.push_back(vocab_->getWordID(word));
    }          
  }
  //cArray.push_back(Vocab::kOOVWordID);  // signify end of corpus for ssarray
  return cArray.size();
}
bool PhraseDictionaryDynSuffixArray::getLocalVocabIDs(const Phrase& src, vector<wordID_t>& localIDs) const {
  // looks up the SA vocab ids for the current src phrase
	size_t phraseSize = src.GetSize();
	for (size_t pos = 0; pos < phraseSize; ++pos) {
		const Word &word = src.GetWord(pos);
		const Factor *factor = word.GetFactor(0);
		std::map<const Factor *, wordID_t>::const_iterator iterLookup;
		iterLookup = vocabLookup_.find(factor);
		
		if (iterLookup == vocabLookup_.end())
      break; 
		else
		{
			wordID_t arrayId = iterLookup->second;
      localIDs.push_back(arrayId);
			cerr << arrayId << " ";
		}
	}
  return (localIDs.size() > 0);
}
TargetPhrase* PhraseDictionaryDynSuffixArray::getMosesFactorIDs(const PhrasePair& phrasepair) const {
  TargetPhrase* targetPhrase = new TargetPhrase(Output);
  int sntIndex = phrasepair.m_sntIndex;
	std::map<wordID_t, const Factor *>::const_iterator rIterLookup;	
  for(int i=phrasepair.m_startTarget; i <= phrasepair.m_endTarget; ++i) { // look up trg words
    rIterLookup = vocabLookupRev_.find(trgCrp_->at(trgSntBreaks_[sntIndex] + i));
    assert(rIterLookup != vocabLookupRev_.end());
    const Factor* factor = rIterLookup->second; 
    Word word;
    word.SetFactor(0, factor);
    targetPhrase->AddWord(word);
  }
  return targetPhrase;
}
const TargetPhraseCollection *PhraseDictionaryDynSuffixArray::GetTargetPhraseCollection(const Phrase& src) const {
	cerr << "\n" << src << "\n";	
	TargetPhraseCollection *ret = new TargetPhraseCollection();
 
	const StaticData &staticData = StaticData::Instance();
  
  vector<wordID_t> localIDs(0), wrdIndices(0);  
  if(!getLocalVocabIDs(src, localIDs)) return ret; 

  std::map<int, pair<int, int> > sntBounds; 
  // extract sentence IDs from SA
  unsigned denom = srcSA_->countPhrase(&localIDs, &wrdIndices, sntBounds);
  vector<int> sntIndexes = getSntIndexes(wrdIndices);	
  for(int snt = 0; snt < sntIndexes.size(); ++snt) {
    vector<PhrasePair*> phrasePairs;
		// extract all Alignments
		int sntIndex = sntIndexes.at(snt);
		//int targetSize = GetTargetSentenceSize(sntIndex);
		const SentenceAlignment &sentenceAlignment = alignments_[sntIndex];
    cerr << "left bnd = " << sntBounds[sntIndex].first << endl;
    cerr << "right bnd = " << sntBounds[sntIndex].second << endl;
    sentenceAlignment.Extract(staticData.GetMaxPhraseLength(), 
															phrasePairs, 
															sntBounds[snt].first, 
															sntBounds[snt].second); 
		
		cerr << "extracted " << phrasePairs.size() << endl;
		
		// convert to moses phrase pairs
		vector<PhrasePair*>::iterator iterPhrasePair;
		for (iterPhrasePair = phrasePairs.begin(); iterPhrasePair != phrasePairs.end(); ++iterPhrasePair)
		{
			const PhrasePair &phrasePair = **iterPhrasePair;
			TargetPhrase *targetPhrase = getMosesFactorIDs(phrasePair); 
			cerr << *targetPhrase << endl;
			
			ret->Add(targetPhrase);
		}
				 
		// done. delete SA phrase pairs
		RemoveAllInColl(phrasePairs);
  }

	return ret;
}
vector<int> PhraseDictionaryDynSuffixArray::getSntIndexes(vector<unsigned>& wrdIndices) const 
{
  vector<unsigned>::const_iterator vit;
  vector<int> sntIndexes(wrdIndices.size()); 
  for(int i = 0; i < wrdIndices.size(); ++i) {
    vit = std::lower_bound(srcSntBreaks_.begin(), srcSntBreaks_.end(), wrdIndices[i]);
    int index = int(vit - srcSntBreaks_.begin());
    sntIndexes.at(i) = index > 0 ? index - 1 : 0;
  }
  return sntIndexes;
}
void PhraseDictionaryDynSuffixArray::save(string fname) {
  // save vocab, SAs, corpus, alignments 
}
void PhraseDictionaryDynSuffixArray::load(string fname) {
  // read vocab, SAs, corpus, alignments 
}
	
SentenceAlignment::SentenceAlignment(int sntIndex, int sourceSize, int targetSize) 
	:m_sntIndex(sntIndex)
	,alignedCountTrg(targetSize, 0)
	,alignedSrc(sourceSize)
{
	for(int i=0; i < targetSize; ++i) {
		vector<int> trgWrd;
		alignedSrc[i] = trgWrd;
	}
}
	
bool SentenceAlignment::Extract(int maxPhraseLength, vector<PhrasePair*> &ret, int startSource, int endSource) const
{
	// foreign = target, F=T
	// english = source, E=S
  int countTarget = alignedCountTrg.size();
	
	int minTarget = 9999;
	int maxTarget = -1;
	vector< int > usedTarget = alignedCountTrg;
	for(int sourcePos = startSource; sourcePos <= endSource; sourcePos++) 
	{
		for(int ind=0; ind < alignedSrc[sourcePos].size();ind++) 
		{
			int targetPos = alignedSrc[sourcePos][ind];
			// cout << "point (" << targetPos << ", " << sourcePos << ")\n";
			if (targetPos<minTarget) { minTarget = targetPos; }
			if (targetPos>maxTarget) { maxTarget = targetPos; }
			usedTarget[ targetPos ]--;
		} // for(int ind=0;ind<sentence
	} // for(int sourcePos=startSource
	
	// cout << "f projected ( " << minTarget << "-" << maxTarget << ", " << startSource << "," << endSource << ")\n"; 
	
	if (maxTarget >= 0 && // aligned to any foreign words at all
			maxTarget-minTarget < maxPhraseLength) 
	{ // foreign phrase within limits
		
		// check if foreign words are aligned to out of bound english words
		bool out_of_bounds = false;
		for(int targetPos=minTarget; targetPos <= maxTarget && !out_of_bounds; targetPos++)
		{
			if (usedTarget[targetPos]>0) 
			{
				// cout << "ouf of bounds: " << targetPos << "\n";
				out_of_bounds = true;
			}
		}
		
		// cout << "doing if for ( " << minTarget << "-" << maxTarget << ", " << startSource << "," << endSource << ")\n"; 
		if (!out_of_bounds)
		{
			// start point of foreign phrase may retreat over unaligned
			for(int startTarget = minTarget;
					(startTarget >= 0 &&
					 startTarget > maxTarget-maxPhraseLength && // within length limit
					 (startTarget==minTarget || alignedCountTrg[startTarget]==0)); // unaligned
					startTarget--)
			{
				// end point of foreign phrase may advance over unaligned
				for (int endTarget=maxTarget;
						 (endTarget<countTarget && 
							endTarget<startTarget+maxPhraseLength && // within length limit
							(endTarget==maxTarget || alignedCountTrg[endTarget]==0)); // unaligned
						 endTarget++)
				{
					PhrasePair *phrasePair = new PhrasePair(m_sntIndex, startTarget,endTarget,startSource,endSource);
					ret.push_back(phrasePair);
				} // for (int endTarget=maxTarget;
			}	// for(int startTarget=minTarget;
		} // if (!out_of_bounds)
	} // if (maxTarget >= 0 &&
	return (ret.size() > 0);
  
}

}// end namepsace
