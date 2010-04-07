#include "PhraseDictionaryDynSuffixArray.h"
#include "DynSAInclude/utils.h"
#include "FactorCollection.h"
#include "StaticData.h"
#include "TargetPhrase.h"
#include <iomanip>

using namespace std;

namespace Moses {
PhraseDictionaryDynSuffixArray::PhraseDictionaryDynSuffixArray(size_t numScoreComponent, 
  PhraseDictionaryFeature* feature): PhraseDictionary(numScoreComponent, feature), 
  maxPhraseLength_(StaticData::Instance().GetMaxPhraseLength()), maxSampleSize_(500) { 
  srcSA_ = 0; 
  trgSA_ = 0;
  srcCrp_ = new vector<wordID_t>();
  trgCrp_ = new vector<wordID_t>();
  vocab_ = new Vocab(false);
  scoreCmp_ = 0;
}
PhraseDictionaryDynSuffixArray::~PhraseDictionaryDynSuffixArray(){
  if(srcSA_) delete srcSA_;
  if(trgSA_) delete trgSA_;
  if(vocab_) delete vocab_;
  if(srcCrp_) delete srcCrp_;
  if(trgCrp_) delete trgCrp_;
  if(scoreCmp_) delete scoreCmp_;
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
  scoreCmp_ = new ScoresComp(m_weight);
	InputFileStream sourceStrme(source);
	InputFileStream targetStrme(target);
  cerr << "Loading source and target parallel corpus...\n";	
	loadCorpus(sourceStrme, *srcCrp_, srcSntBreaks_);
  loadCorpus(targetStrme, *trgCrp_, trgSntBreaks_);
  assert(srcSntBreaks_.size() == trgSntBreaks_.size());
	LoadVocabLookup();

  // build suffix arrays and auxilliary arrays
  cerr << "Building Source Suffix Array...\n"; 
  srcSA_ = new DynSuffixArray(srcCrp_); 
  if(!srcSA_) return false;
  /*cerr << "Building Target Suffix Array...\n"; 
  trgSA_ = new DynSuffixArray(trgCrp_); 
  if(!trgSA_) return false;*/
	
	InputFileStream alignStrme(alignments);
  cerr << "Loading Alignment File...\n"; 
  loadRawAlignments(alignStrme);
  //loadAlignments(alignStrme);
  return true;
}
int PhraseDictionaryDynSuffixArray::loadRawAlignments(InputFileStream& align) {
  // stores the alignments in the raw file format 
  string line;
  vector<int> vtmp;
  while(getline(align, line)) {
    Utils::splitToInt(line, vtmp, "- ");
    assert(vtmp.size() % 2 == 0);
    vector<short> vAlgn;  // store as short ints for memory
    iterate(vtmp, itr) vAlgn.push_back(short(*itr));
    rawAlignments_.push_back(vAlgn);
  }
  return rawAlignments_.size();
}
int PhraseDictionaryDynSuffixArray::loadAlignments(InputFileStream& align) {
  string line;
  vector<int> vtmp;
  int sntIndex(0);
	
  while(getline(align, line)) {
    Utils::splitToInt(line, vtmp, "- ");
    assert(vtmp.size() % 2 == 0);
		
		int sourceSize = GetSourceSentenceSize(sntIndex);
		int targetSize = GetTargetSentenceSize(sntIndex);

    SentenceAlignment curSnt(sntIndex, sourceSize, targetSize); // initialize empty sentence 
    for(int i=0; i < (int)vtmp.size(); i+=2) {
			int sourcePos = vtmp[i];
			int targetPos = vtmp[i+1];
			assert(sourcePos < sourceSize);
			assert(targetPos < targetSize);
			
      curSnt.alignedList[sourcePos].push_back(targetPos);  // list of target nodes for each source word 
      curSnt.numberAligned[targetPos]++; // cnt of how many source words connect to this target word 
    }
    curSnt.srcSnt = srcCrp_ + sntIndex;  // point source and target sentence
    curSnt.trgSnt = trgCrp_ + sntIndex; 
    alignments_.push_back(curSnt);
		
		sntIndex++;
  }
  return alignments_.size();
}
SentenceAlignment PhraseDictionaryDynSuffixArray::getSentenceAlignment(const int sntIndex, bool trg2Src) const {
  // retrieves the alignments in the format used by SentenceAlignment.extract()
  int sntGiven = trg2Src ? GetTargetSentenceSize(sntIndex) : GetSourceSentenceSize(sntIndex);
  int sntExtract = trg2Src ? GetSourceSentenceSize(sntIndex) : GetTargetSentenceSize(sntIndex);
  vector<short> alignment = rawAlignments_.at(sntIndex);
  SentenceAlignment curSnt(sntIndex, sntGiven, sntExtract); // initialize empty sentence 
  for(int i=0; i < alignment.size(); i+=2) {
    int sourcePos = alignment[i];
    int targetPos = alignment[i+1];
	  if(trg2Src) {
      curSnt.alignedList[targetPos].push_back(sourcePos);  // list of target nodes for each source word 
      curSnt.numberAligned[sourcePos]++; // cnt of how many source words connect to this target word 
    }
    else {
      curSnt.alignedList[sourcePos].push_back(targetPos);  // list of target nodes for each source word 
      curSnt.numberAligned[targetPos]++; // cnt of how many source words connect to this target word 
    }
  }
  curSnt.srcSnt = srcCrp_ + sntIndex;  // point source and target sentence
  curSnt.trgSnt = trgCrp_ + sntIndex; 
		
  return curSnt;
}
bool PhraseDictionaryDynSuffixArray::extractPhrases(const int& sntIndex, const int& wordIndex,  
  const int& sourceSize, vector<PhrasePair*>& phrasePairs, bool trg2Src) const {
  /* extractPhrases() can extract the matching phrases for both directions by using the trg2Src 
   * parameter */
  SentenceAlignment curSnt = getSentenceAlignment(sntIndex, trg2Src);
  // get span of phrase in source sentence 
  int beginSentence = srcSntBreaks_[sntIndex];
  int rightIdx = wordIndex - beginSentence
      ,leftIdx = rightIdx - sourceSize + 1;
  return curSnt.Extract(maxPhraseLength_, phrasePairs, leftIdx, rightIdx); // extract all phrase Alignments in sentence
}
void PhraseDictionaryDynSuffixArray::LoadVocabLookup()
{
	FactorCollection &factorCollection = FactorCollection::Instance();
	
	Vocab::Word2Id::const_iterator iter;
	for (iter = vocab_->vocabStart(); iter != vocab_->vocabEnd(); ++iter)
	{
		const word_t &str = iter->first;
		wordID_t arrayId = iter->second;
		//const Factor *factor = factorCollection.AddFactor(Input, 0, str, false);
		const Factor *factor = factorCollection.AddFactor(Input, 0, str);
		vocabLookup_[factor] = arrayId;
		vocabLookupRev_[arrayId] = factor;
	}
			
}
void PhraseDictionaryDynSuffixArray::InitializeForInput(const InputType& input)
{
  assert(&input == &input);
}
void PhraseDictionaryDynSuffixArray::CleanUp() {
  wordPairCache_.clear();
}
void PhraseDictionaryDynSuffixArray::SetWeightTransModel(const std::vector<float, std::allocator<float> >&) {
  return;
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
  //cArray.push_back(Vocab::kOOVWordID);  // signify end of corpus 
  return cArray.size();
}
bool PhraseDictionaryDynSuffixArray::getLocalVocabIDs(const Phrase& src, SAPhrase &output) const {
  // looks up the SA vocab ids for the current src phrase
	size_t phraseSize = src.GetSize();
	for (size_t pos = 0; pos < phraseSize; ++pos) {
		const Word &word = src.GetWord(pos);
		const Factor *factor = word.GetFactor(0);
		std::map<const Factor *, wordID_t>::const_iterator iterLookup;
		iterLookup = vocabLookup_.find(factor);
		
		if (iterLookup == vocabLookup_.end())
		{ // oov
      return false;
		}
		else
		{
			wordID_t arrayId = iterLookup->second;
      output.SetId(pos, arrayId);
			//cerr << arrayId << " ";
		}
	}
  return true;
}
pair<float, float> PhraseDictionaryDynSuffixArray::getLexicalWeight(const PhrasePair& phrasepair) const {
  float srcLexWeight(1.0), trgLexWeight(1.0);
  map<pair<wordID_t, wordID_t>, float> targetProbs; // collect sum of target probs given source words
  //const SentenceAlignment& alignment = alignments_[phrasepair.m_sntIndex];
  const SentenceAlignment& alignment = getSentenceAlignment(phrasepair.m_sntIndex);
  std::map<pair<wordID_t, wordID_t>, pair<float, float> >::const_iterator itrCache; 
  // for each source word
  for(int srcIdx = phrasepair.m_startSource; srcIdx <= phrasepair.m_endSource; ++srcIdx) {
    float srcSumPairProbs(0);
    wordID_t srcWord = srcCrp_->at(srcIdx + srcSntBreaks_[phrasepair.m_sntIndex]);  // localIDs
    const vector<int>& srcWordAlignments = alignment.alignedList.at(srcIdx);
    if(srcWordAlignments.size() == 0) { // get p(NULL|src)
      pair<wordID_t, wordID_t> wordpair = std::make_pair(srcWord, Vocab::kOOVWordID);
      itrCache = wordPairCache_.find(wordpair);
      if(itrCache == wordPairCache_.end()) { // if not in cache
        cacheWordProbs(srcWord);
        itrCache = wordPairCache_.find(wordpair); // search cache again
      }
      assert(itrCache != wordPairCache_.end());
      srcSumPairProbs += itrCache->second.first;
      targetProbs[wordpair] = itrCache->second.second;
    }
    else { // extract p(trg|src) 
      for(int i = 0; i < srcWordAlignments.size(); ++i) { // for each aligned word
        int trgIdx = srcWordAlignments[i];
        wordID_t trgWord = trgCrp_->at(trgIdx + trgSntBreaks_[phrasepair.m_sntIndex]);
        // get probability of this source->target word pair
        pair<wordID_t, wordID_t> wordpair = std::make_pair(srcWord, trgWord);
        itrCache = wordPairCache_.find(wordpair);
        if(itrCache == wordPairCache_.end()) { // if not in cache
          cacheWordProbs(srcWord);
          itrCache = wordPairCache_.find(wordpair); // search cache again
        }
        assert(itrCache != wordPairCache_.end());
        srcSumPairProbs += itrCache->second.first;
        targetProbs[wordpair] = itrCache->second.second;  
      } 
    }
    float srcNormalizer = srcWordAlignments.size() < 2 ? 1.0 : 1.0 / float(srcWordAlignments.size());
    srcLexWeight *= (srcNormalizer * srcSumPairProbs);  
  }  // end for each source word
  for(int trgIdx = phrasepair.m_startTarget; trgIdx <= phrasepair.m_endTarget; ++trgIdx) {
    float trgSumPairProbs(0);
    wordID_t trgWord = trgCrp_->at(trgIdx + trgSntBreaks_[phrasepair.m_sntIndex]);
    iterate(targetProbs, trgItr) {
      if(trgItr->first.second == trgWord) 
        trgSumPairProbs += trgItr->second;
    }
    if(trgSumPairProbs == 0) continue;  // currently don't store target-side SA
    int noAligned = alignment.numberAligned.at(trgIdx);
    float trgNormalizer = noAligned < 2 ? 1.0 : 1.0 / float(noAligned);
    trgLexWeight *= (trgNormalizer * trgSumPairProbs);
  }
  // TODO::Need to get p(NULL|trg)
  return pair<float, float>(srcLexWeight, trgLexWeight);
}
void PhraseDictionaryDynSuffixArray::cacheWordProbs(wordID_t srcWord) const {
  std::map<wordID_t, int> counts;
  vector<wordID_t> vword(1, srcWord), wrdIndices;  
  assert(srcSA_->getCorpusIndex(&vword, &wrdIndices));
  vector<int> sntIndexes = getSntIndexes(wrdIndices, 1);	
  float denom(0);
  // for each occurrence of this word 
  for(int snt = 0; snt < sntIndexes.size(); ++snt) {
    int sntIdx = sntIndexes.at(snt); // get corpus index for sentence
    assert(sntIdx != -1); 
    int srcWrdSntIdx = wrdIndices.at(snt) - srcSntBreaks_.at(sntIdx); // get word index in sentence
    const vector<int> srcAlg = getSentenceAlignment(sntIdx).alignedList.at(srcWrdSntIdx); // list of target words for this source word
    //const vector<int>& srcAlg = alignments_.at(sntIdx).alignedList.at(srcWrdSntIdx); // list of target words for this source word
    if(srcAlg.size() == 0) {
      ++counts[Vocab::kOOVWordID]; // if not alligned then align to NULL word
      ++denom;
    }
    else { //get target words aligned to srcword in this sentence
      for(int i=0; i < srcAlg.size(); ++i) {
        wordID_t trgWord = trgCrp_->at(srcAlg[i] + trgSntBreaks_[sntIdx]);
        ++counts[trgWord];
        ++denom;
      }
    }
  }
  // now we've gotten counts of all target words aligned to this source word
  // get probs and cache all pairs
  for(std::map<wordID_t, int>::const_iterator itrCnt = counts.begin();
      itrCnt != counts.end(); ++itrCnt) {
    pair<wordID_t, wordID_t> wordPair = std::make_pair(srcWord, itrCnt->first);
    float srcTrgPrb = float(itrCnt->second) / float(denom);  // gives p(src->trg)
    float trgSrcPrb = float(itrCnt->second) / float(counts.size()); // gives p(trg->src) 
    wordPairCache_[wordPair] = pair<float, float>(srcTrgPrb, trgSrcPrb);
  }
}
SAPhrase PhraseDictionaryDynSuffixArray::trgPhraseFromSntIdx(const PhrasePair& phrasepair) const {
// takes sentence indexes and looks up vocab IDs
  SAPhrase phraseIds(phrasepair.GetTargetSize());
  int sntIndex = phrasepair.m_sntIndex;
  int id(-1), pos(0);
  for(int i=phrasepair.m_startTarget; i <= phrasepair.m_endTarget; ++i) { // look up trg words
    id = trgCrp_->at(trgSntBreaks_[sntIndex] + i);
    phraseIds.SetId(pos++, id);
  }
  return phraseIds;
}
	
TargetPhrase* PhraseDictionaryDynSuffixArray::getMosesFactorIDs(const SAPhrase& phrase) const {
  TargetPhrase* targetPhrase = new TargetPhrase(Output);
	std::map<wordID_t, const Factor *>::const_iterator rIterLookup;	
  for(int i=0; i < phrase.words.size(); ++i) { // look up trg words
    rIterLookup = vocabLookupRev_.find(phrase.words[i]);
    assert(rIterLookup != vocabLookupRev_.end());
    const Factor* factor = rIterLookup->second; 
    Word word;
    word.SetFactor(0, factor);
    targetPhrase->AddWord(word);
  }
	
	// scoring
  return targetPhrase;
}
const TargetPhraseCollection *PhraseDictionaryDynSuffixArray::GetTargetPhraseCollection(const Phrase& src) const {
	//cout << "\n" << src << "\n";	
	TargetPhraseCollection *ret = new TargetPhraseCollection();
	size_t sourceSize = src.GetSize();
	SAPhrase localIDs(sourceSize);
  if(!getLocalVocabIDs(src, localIDs)) return ret; 
	float totalTrgPhrases(0); 
  std::map<SAPhrase, int> phraseCounts;
  std::map<SAPhrase, pair<float, float> > lexicalWeights;
  std::map<SAPhrase, pair<float, float> >::iterator itrLexW;
  vector<unsigned> wrdIndices(0);  
  // extract sentence IDs from SA and return rightmost index of phrases
  if(!srcSA_->getCorpusIndex(&(localIDs.words), &wrdIndices)) return ret;
  if(wrdIndices.size() > maxSampleSize_) 
    wrdIndices = sampleSelection(wrdIndices);
  vector<int> sntIndexes = getSntIndexes(wrdIndices, sourceSize);	
  // for each sentence with this phrase
  for(int snt = 0; snt < sntIndexes.size(); ++snt) {
    vector<PhrasePair*> phrasePairs; // to store all phrases possible from current sentence
		int sntIndex = sntIndexes.at(snt); // get corpus index for sentence
    if(sntIndex == -1) continue;  // bad flag set by getSntIndexes()
    extractPhrases(sntIndex, wrdIndices[snt], sourceSize, phrasePairs); 
		//cerr << "extracted " << phrasePairs.size() << endl;
    totalTrgPhrases += phrasePairs.size(); // keep track of count of each extracted phrase pair  	
		vector<PhrasePair*>::iterator iterPhrasePair;
		for (iterPhrasePair = phrasePairs.begin(); iterPhrasePair != phrasePairs.end(); ++iterPhrasePair) {
      SAPhrase phrase = trgPhraseFromSntIdx(**iterPhrasePair);
      phraseCounts[phrase]++;  // count each unique phrase
      pair<float, float> lexWeight = getLexicalWeight(**iterPhrasePair);  // get lexical weighting for this phrase pair 
      itrLexW = lexicalWeights.find(phrase); // check if phrase already has lexical weight attached
      if((itrLexW != lexicalWeights.end()) && (itrLexW->second.first < lexWeight.first)) 
        itrLexW->second = lexWeight;  // if this lex weight is greater save it
      else lexicalWeights[phrase] = lexWeight; // else save 
		}
		// done with sentence. delete SA phrase pairs
		RemoveAllInColl(phrasePairs);
  } // done with all sentences
  // convert to moses phrase pairs
  const int maxReturn = 20;
  std::map<SAPhrase, int>::const_iterator iterPhrases; 
  std::multimap<Scores, const SAPhrase*, ScoresComp> phraseScores (*scoreCmp_);
  // get scores of all phrases
  for(iterPhrases = phraseCounts.begin(); iterPhrases != phraseCounts.end(); ++iterPhrases) {
    float trg2SrcMLE = float(iterPhrases->second) / totalTrgPhrases;
    itrLexW = lexicalWeights.find(iterPhrases->first);
    assert(itrLexW != lexicalWeights.end());
    Scores scoreVector(3);
    scoreVector[0] = trg2SrcMLE; 
    scoreVector[1] = itrLexW->second.first;
    scoreVector[2] = 2.718; // exp(1); 
    phraseScores.insert(pair<Scores, const SAPhrase*>(scoreVector, &iterPhrases->first));
  }
  // return top scoring phrases
  std::multimap<Scores, const SAPhrase*, ScoresComp>::reverse_iterator ritr;
  for(ritr = phraseScores.rbegin(); ritr != phraseScores.rend(); ++ritr) {
    Scores scoreVector = ritr->first;
    Moses::TargetPhrase *targetPhrase = getMosesFactorIDs(*ritr->second);
    //std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),NegateScore);
    std::transform(scoreVector.begin(),scoreVector.end(),scoreVector.begin(),FloorScore);
    targetPhrase->SetScore(m_feature, scoreVector, m_weight, m_weightWP, *m_languageModels);
    //cout << *targetPhrase << "\t" << std::setprecision(8) << scoreVector[2] << endl;
    ret->Add(targetPhrase);
    if(ret->GetSize() == maxReturn) break;
  }
  ret->NthElement(m_tableLimit); // sort the phrases for the dcoder
	return ret;
}
vector<int> PhraseDictionaryDynSuffixArray::getSntIndexes(vector<unsigned>& wrdIndices, 
  const int sourceSize) const 
{
  vector<unsigned>::const_iterator vit;
  vector<int> sntIndexes; 
  for(int i=0; i < wrdIndices.size(); ++i) {
    vit = std::upper_bound(srcSntBreaks_.begin(), srcSntBreaks_.end(), wrdIndices[i]);
    int index = int(vit - srcSntBreaks_.begin()) - 1;
    // check for phrases that cross sentence boundaries
    if(wrdIndices[i] - sourceSize + 1 < srcSntBreaks_.at(index)) 
      sntIndexes.push_back(-1);  // set bad flag
    else
      sntIndexes.push_back(index);  // store the index of the sentence in the corpus
  }
  return sntIndexes;
}
vector<unsigned> PhraseDictionaryDynSuffixArray::sampleSelection(vector<unsigned> sample) const {
  int size = sample.size();
  //if(size < maxSampleSize_) return sample;
  vector<unsigned> subSample;
  int jump = size / maxSampleSize_;
  for(int i=0; i < size; i+=jump)
   subSample.push_back(sample.at(i));
  return subSample;
}
void PhraseDictionaryDynSuffixArray::save(string fname) {
  // save vocab, SAs, corpus, alignments 
}
void PhraseDictionaryDynSuffixArray::load(string fname) {
  // read vocab, SAs, corpus, alignments 
}
	
SentenceAlignment::SentenceAlignment(int sntIndex, int sourceSize, int targetSize) 
	:m_sntIndex(sntIndex)
	,numberAligned(targetSize, 0)
	,alignedList(sourceSize)
{
	for(int i=0; i < sourceSize; ++i) {
		vector<int> trgWrd;
		alignedList[i] = trgWrd;
	}
}
bool SentenceAlignment::Extract(int maxPhraseLength, vector<PhrasePair*> &ret, int startSource, int endSource) const
{
	// foreign = target, F=T
	// english = source, E=S
  int countTarget = numberAligned.size();
	
	int minTarget = 9999;
	int maxTarget = -1;
	vector< int > usedTarget = numberAligned;
	for(int sourcePos = startSource; sourcePos <= endSource; sourcePos++) 
	{
		for(int ind=0; ind < (int)alignedList[sourcePos].size();ind++) 
		{
			int targetPos = alignedList[sourcePos][ind];
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
					 (startTarget==minTarget || numberAligned[startTarget]==0)); // unaligned
					startTarget--)
			{
				// end point of foreign phrase may advance over unaligned
				for (int endTarget=maxTarget;
						 (endTarget<countTarget && 
							endTarget<startTarget+maxPhraseLength && // within length limit
							(endTarget==maxTarget || numberAligned[endTarget]==0)); // unaligned
						 endTarget++)
				{
					PhrasePair *phrasePair = new PhrasePair(startTarget,endTarget,startSource,endSource, m_sntIndex);
					ret.push_back(phrasePair);
				} // for (int endTarget=maxTarget;
			}	// for(int startTarget=minTarget;
		} // if (!out_of_bounds)
	} // if (maxTarget >= 0 &&
	return (ret.size() > 0);
  
}

}// end namepsace
