#include "BilingualDynSuffixArray.h"
#include "moses/TranslationModel/DynSAInclude/utils.h"
#include "moses/FactorCollection.h"
#include "moses/StaticData.h"
#include "moses/TargetPhrase.h"

#include "moses/TranslationModel/UG/generic/sorting/NBestList.h"
#include "moses/TranslationModel/UG/generic/sampling/Sampling.h"

#include <boost/foreach.hpp>
#include <iomanip>

using namespace std;

namespace Moses
{

BilingualDynSuffixArray::
BilingualDynSuffixArray():
  m_maxPhraseLength(StaticData::Instance().GetMaxPhraseLength()),
  m_maxSampleSize(20), m_maxPTEntries(20)
{
  m_srcSA = 0;
  m_trgSA = 0;
  m_srcCorpus = new vector<wordID_t>();
  m_trgCorpus = new vector<wordID_t>();
  m_srcVocab  = new Vocab(false);
  m_trgVocab  = new Vocab(false);
  m_scoreCmp = 0;
}

BilingualDynSuffixArray::
~BilingualDynSuffixArray()
{
  if(m_srcSA)     delete m_srcSA;
  if(m_trgSA)     delete m_trgSA;
  if(m_srcVocab)  delete m_srcVocab;
  if(m_trgVocab)  delete m_trgVocab;
  if(m_srcCorpus) delete m_srcCorpus;
  if(m_trgCorpus) delete m_trgCorpus;
  if(m_scoreCmp)  delete m_scoreCmp;
}

bool
BilingualDynSuffixArray::
Load(
  const vector<FactorType>& inputFactors,
  const vector<FactorType>& outputFactors,
  string source, string target, string alignments,
  const vector<float> &weight)
{
  m_inputFactors = inputFactors;
  m_outputFactors = outputFactors;

  // m_scoreCmp = new ScoresComp(weight);
  InputFileStream sourceStrme(source);
  InputFileStream targetStrme(target);
  cerr << "Loading source corpus...\n";
  // Input and Output are 'Factor directions' (whatever that is) defined in Typedef.h
  LoadCorpus(Input, sourceStrme, m_inputFactors, *m_srcCorpus, m_srcSntBreaks, m_srcVocab);
  cerr << "Loading target corpus...\n";
  LoadCorpus(Output, targetStrme, m_outputFactors,*m_trgCorpus, m_trgSntBreaks, m_trgVocab);

  UTIL_THROW_IF2(m_srcSntBreaks.size() != m_trgSntBreaks.size(),
		  "Source and target arrays aren't the same size");

  // build suffix arrays and auxilliary arrays
  cerr << "Building Source Suffix Array...\n";
  m_srcSA = new DynSuffixArray(m_srcCorpus);
  if(!m_srcSA) return false;
  cerr << "Building Target Suffix Array...\n";
  m_trgSA = new DynSuffixArray(m_trgCorpus);
  if(!m_trgSA) return false;

  InputFileStream alignStrme(alignments);
  cerr << "Loading Alignment File...\n";
  LoadRawAlignments(alignStrme);
  cerr << m_srcSntBreaks.size() << " "
       << m_trgSntBreaks.size() << " "
       << m_rawAlignments.size() << endl;
  //LoadAlignments(alignStrme);
  cerr << "Building frequent word cache...\n";
  CacheFreqWords();

  wordID_t const* s = &(*m_srcCorpus)[0];
  wordID_t const* t = &(*m_trgCorpus)[0];
  for (size_t sid = 0; sid < m_srcSntBreaks.size(); ++sid) {
    wordID_t const* se = s + GetSourceSentenceSize(sid);
    wordID_t const* te = t + GetTargetSentenceSize(sid);
    vector<short> const& a = m_rawAlignments[sid];
    m_wrd_cooc.Count(vector<wordID_t>(s,se),
                     vector<wordID_t>(t,te), a,
                     m_srcVocab->GetkOOVWordID(),
                     m_trgVocab->GetkOOVWordID());
    s = se;
    t = te;
  }
  if (m_srcSntBreaks.size()  != m_trgSntBreaks.size() ||
      m_rawAlignments.size() != m_trgSntBreaks.size()) {
    cerr << "FATAL ERROR: Line counts don't match!\n"
         << "Source side text corpus: " << m_srcSntBreaks.size() << "\n"
         << "Target side text corpus: " << m_trgSntBreaks.size() << "\n"
         << "Word alignments:         " << m_rawAlignments.size() << endl;
    exit(1);
  }
  return true;
}

int
BilingualDynSuffixArray::
LoadRawAlignments(InputFileStream& align)
{
  // stores the alignments in the raw file format
  string line;
  // vector<int> vtmp;
  // int lineNum = 0;
  while(getline(align, line)) {
    // if (++lineNum % 10000 == 0) cerr << lineNum << endl;
    LoadRawAlignments(line);
  }
  return m_rawAlignments.size();
}


int
BilingualDynSuffixArray::
LoadRawAlignments(string& align)
{
  // stores the alignments in the raw file format
  vector<int> vtmp;
  Utils::splitToInt(align, vtmp, "- ");
  UTIL_THROW_IF2(vtmp.size() % 2 != 0,
		  "Alignment format is incorrect: " << align);
  vector<short> vAlgn;  // store as short ints for memory
  for (vector<int>::const_iterator itr = vtmp.begin();
       itr != vtmp.end(); ++itr) {
    vAlgn.push_back(short(*itr));
  }
  m_rawAlignments.push_back(vAlgn);
  return m_rawAlignments.size();
}

SentenceAlignment
BilingualDynSuffixArray::
GetSentenceAlignment(const int sntIndex, bool trg2Src) const
{
  // retrieves the alignments in the format used by SentenceAlignment.Extract()
  int t = GetTargetSentenceSize(sntIndex);
  int s = GetSourceSentenceSize(sntIndex);
  int sntGiven   = trg2Src ? t : s;
  int sntExtract = trg2Src ? s : t;
  SentenceAlignment curSnt(sntIndex, sntGiven, sntExtract); // initialize empty sentence
  vector<short> const& a = m_rawAlignments.at(sntIndex);
  for(size_t i=0; i < a.size(); i+=2) {
    int sourcePos = a[i];
    int targetPos = a[i+1];
    if(trg2Src) {
      curSnt.alignedList[targetPos].push_back(sourcePos);	// list of target nodes for each source word
      curSnt.numberAligned[sourcePos]++; // cnt of how many source words connect to this target word
    } else {
      curSnt.alignedList[sourcePos].push_back(targetPos);	// list of target nodes for each source word
      curSnt.numberAligned[targetPos]++; // cnt of how many source words connect to this target word
    }
  }
  curSnt.srcSnt = m_srcCorpus + sntIndex;	// point source and target sentence
  curSnt.trgSnt = m_trgCorpus + sntIndex;

  return curSnt;
}

bool
BilingualDynSuffixArray::
ExtractPhrases(const int& sntIndex,
               const int& wordIndex,
               const int& sourceSize,
               vector<PhrasePair*>& phrasePairs,
               bool trg2Src) const
{
  /* ExtractPhrases() can extract the matching phrases for both directions by using the trg2Src
   * parameter */
  SentenceAlignment curSnt = GetSentenceAlignment(sntIndex, trg2Src);
  // get span of phrase in source sentence
  int beginSentence = m_srcSntBreaks[sntIndex];
  int rightIdx = wordIndex - beginSentence;
  int leftIdx  = rightIdx  - sourceSize + 1;
  return curSnt.Extract(m_maxPhraseLength, phrasePairs, leftIdx, rightIdx); // extract all phrase Alignments in sentence
}

void
BilingualDynSuffixArray::
CleanUp(const InputType& source)
{
  //m_wordPairCache.clear();
}

int
BilingualDynSuffixArray::
LoadCorpus(FactorDirection direction,
           InputFileStream  & corpus,
           const FactorList & factors,
           vector<wordID_t> & cArray,
           vector<wordID_t> & sntArray,
           Vocab* vocab)
{
  string line, word;
  int sntIdx(0);
  // corpus.seekg(0); Seems needless -> commented out to allow
  // loading of gzipped corpora (gzfilebuf doesn't support seeking).
  const string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  while(getline(corpus, line)) {
    sntArray.push_back(sntIdx);
    Phrase phrase(ARRAY_SIZE_INCR);
    // parse phrase
    phrase.CreateFromString( direction, factors, line, factorDelimiter, NULL);
    // store words in vocabulary and corpus
    for( size_t i = 0; i < phrase.GetSize(); ++i) {
      cArray.push_back( vocab->GetWordID(phrase.GetWord(i)) );
    }
    sntIdx += phrase.GetSize();
  }
  //cArray.push_back(vocab->GetkOOVWordID);	// signify end of corpus
  vocab->MakeClosed(); // avoid adding words
  return cArray.size();
}

bool
BilingualDynSuffixArray::
GetLocalVocabIDs(const Phrase& src, SAPhrase &output) const
{
  // looks up the SA vocab ids for the current src phrase
  size_t phraseSize = src.GetSize();
  for (size_t pos = 0; pos < phraseSize; ++pos) {
    const Word &word = src.GetWord(pos);
    wordID_t arrayId = m_srcVocab->GetWordID(word);
    if (arrayId == m_srcVocab->GetkOOVWordID()) {
      // oov
      return false;
    } else {
      output.SetId(pos, arrayId);
    }
  }
  return true;
}

pair<float, float>
BilingualDynSuffixArray::
GetLexicalWeight(const PhrasePair& pp) const
{
  // sp,tp: sum of link probabilities
  // sc,tc: count of links
  int src_size = pp.GetSourceSize();
  int trg_size = pp.GetTargetSize();
  vector<float> sp(src_size, 0), tp(trg_size, 0);
  vector<int>   sc(src_size,0),  tc(trg_size,0);
  wordID_t const* sw = &(m_srcCorpus->at(m_srcSntBreaks.at(pp.m_sntIndex)));
  wordID_t const* tw = &(m_trgCorpus->at(m_trgSntBreaks.at(pp.m_sntIndex)));
  vector<short> const & a = m_rawAlignments.at(pp.m_sntIndex);
  for (size_t i = 0; i < a.size(); i += 2) {
    int s = a[i], t = a.at(i+1), sx, tx;
    // sx, tx: local positions within phrase pair

    if (s < pp.m_startSource || t < pp.m_startTarget) continue;
    if ((sx = s - pp.m_startSource) >= src_size) continue;
    if ((tx = t - pp.m_startTarget) >= trg_size) continue;

    sp[sx] += m_wrd_cooc.pfwd(sw[s],tw[t]);
    tp[tx] += m_wrd_cooc.pbwd(sw[s],tw[t]);
    ++sc[sx];
    ++tc[tx];
#if 0
    cout << m_srcVocab->GetWord(sw[s])   << " -> "
         << m_trgVocab->GetWord(tw[t])   << " "
         << m_wrd_cooc.pfwd(sw[s],tw[t]) << " "
         << m_wrd_cooc.pbwd(sw[s],tw[t]) << " "
         << sp[sx] << " (" << sc[sx] << ") "
         << tp[tx] << " (" << tc[tx] << ") "
         << endl;
#endif
  }
  pair<float,float> ret(1,1);
  wordID_t null_trg = m_trgVocab->GetkOOVWordID();
  wordID_t null_src = m_srcVocab->GetkOOVWordID();
  size_t soff = pp.m_startSource;
  for (size_t i = 0; i < sp.size(); ++i) {
    if (sc[i]) ret.first *= sp[i]/sc[i];
    else       ret.first *= m_wrd_cooc.pfwd(sw[soff+i], null_trg);
  }
  size_t toff = pp.m_startTarget;
  for (size_t i = 0; i < tp.size(); ++i) {
    if (tc[i]) ret.second *= tp[i]/tc[i];
    else       ret.second *= m_wrd_cooc.pbwd(null_src,tw[toff+i]);
  }
  return ret;
}

void
BilingualDynSuffixArray::
CacheFreqWords() const
{
  multimap<int, wordID_t> wordCnts;
  // for each source word in vocab
  Vocab::Word2Id::const_iterator it;
  for(it = m_srcVocab->VocabStart(); it != m_srcVocab->VocabEnd(); ++it) {
    // get its frequency
    wordID_t srcWord = it->second;
    vector<wordID_t> sword(1, srcWord), wrdIndices;
    m_srcSA->GetCorpusIndex(&sword, &wrdIndices);
    if(wrdIndices.size() >= 1000) { // min count
      wordCnts.insert(make_pair(wrdIndices.size(), srcWord));
    }
  }
  int numSoFar(0);
  multimap<int, wordID_t>::reverse_iterator ritr;
  for(ritr = wordCnts.rbegin(); ritr != wordCnts.rend(); ++ritr) {
    m_freqWordsCached.insert(ritr->second);
    CacheWordProbs(ritr->second);
    if(++numSoFar == 50) break; // get top counts
  }
  cerr << "\tCached " << m_freqWordsCached.size() << " source words\n";
}

void
BilingualDynSuffixArray::
CacheWordProbs(wordID_t srcWord) const
{
  map<wordID_t, int> counts;
  vector<wordID_t> sword(1, srcWord), wrdIndices;
  bool ret = m_srcSA->GetCorpusIndex(&sword, &wrdIndices);
  UTIL_THROW_IF2(!ret, "Error");

  vector<int> sntIndexes = GetSntIndexes(wrdIndices, 1, m_srcSntBreaks);
  float denom(0);
  // for each occurrence of this word
  for(size_t snt = 0; snt < sntIndexes.size(); ++snt) {
    int sntIdx = sntIndexes.at(snt); // get corpus index for sentence
    UTIL_THROW_IF2(sntIdx == -1, "Error");

    int srcWrdSntIdx = wrdIndices.at(snt) - m_srcSntBreaks.at(sntIdx); // get word index in sentence
    const vector<int> srcAlg = GetSentenceAlignment(sntIdx).alignedList.at(srcWrdSntIdx); // list of target words for this source word
    if(srcAlg.size() == 0) {
      ++counts[m_srcVocab->GetkOOVWordID()]; // if not alligned then align to NULL word
      ++denom;
    } else { //get target words aligned to srcword in this sentence
      for(size_t i=0; i < srcAlg.size(); ++i) {
        wordID_t trgWord = m_trgCorpus->at(srcAlg[i] + m_trgSntBreaks[sntIdx]);
        ++counts[trgWord];
        ++denom;
      }
    }
  }
  // now we've gotten counts of all target words aligned to this source word
  // get probs and cache all pairs
  for(map<wordID_t, int>::const_iterator itrCnt = counts.begin();
      itrCnt != counts.end(); ++itrCnt) {
    pair<wordID_t, wordID_t> wordPair = make_pair(srcWord, itrCnt->first);
    float srcTrgPrb = float(itrCnt->second) / float(denom);	// gives p(src->trg)
    float trgSrcPrb = float(itrCnt->second) / float(counts.size()); // gives p(trg->src)
    m_wordPairCache[wordPair] = pair<float, float>(srcTrgPrb, trgSrcPrb);
  }
}

SAPhrase
BilingualDynSuffixArray::
TrgPhraseFromSntIdx(const PhrasePair& phrasepair) const
{
  // takes sentence indexes and looks up vocab IDs
  SAPhrase phraseIds(phrasepair.GetTargetSize());
  int sntIndex = phrasepair.m_sntIndex;
  int id(-1), pos(0);
  for(int i=phrasepair.m_startTarget; i <= phrasepair.m_endTarget; ++i) { // look up trg words
    id = m_trgCorpus->at(m_trgSntBreaks[sntIndex] + i);
    phraseIds.SetId(pos++, id);
  }
  return phraseIds;
}

TargetPhrase*
BilingualDynSuffixArray::
GetMosesFactorIDs(const SAPhrase& phrase, const Phrase& sourcePhrase) const
{
  TargetPhrase* targetPhrase = new TargetPhrase();
  for(size_t i=0; i < phrase.words.size(); ++i) { // look up trg words
    Word& word = m_trgVocab->GetWord( phrase.words[i]);
    UTIL_THROW_IF2(word == m_trgVocab->GetkOOVWord(),
    		"Unknown word at position " << i);
    targetPhrase->AddWord(word);
  }
  // scoring
  return targetPhrase;
}

/// Gather translation candidates for source phrase /src/ and store raw
//  phrase pair statistics in /pstats/. Return the sample rate
//  (number of samples considered / total number of hits) and total number of
//  phrase pairs
pair<float,float>
BilingualDynSuffixArray::
GatherCands(Phrase const& src, map<SAPhrase, vector<float> >& pstats) const
{
  typedef map<SAPhrase, vector<float> >::iterator   pstat_iter;
  typedef map<SAPhrase, vector<float> >::value_type pstat_entry;
  pair<float,float> ret(0,0);
  float& sampleRate   = ret.first;
  float& totalPhrases = ret.second;
  size_t srcSize = src.GetSize();
  SAPhrase localIDs(srcSize);
  vector<unsigned> wrdIndices;
  if(!GetLocalVocabIDs(src, localIDs) ||
      !m_srcSA->GetCorpusIndex(&(localIDs.words), &wrdIndices))
    return ret; // source phrase contains OOVs

  // select a sample of the occurrences for phrase extraction
  size_t m1 = wrdIndices.size();
  SampleSelection(wrdIndices); // careful! SampleSelection alters wrdIndices!
  sampleRate = float(wrdIndices.size())/m1;

  // determine the sentences in which these phrases occur
  vector<int> sntIndices = GetSntIndexes(wrdIndices, srcSize, m_srcSntBreaks);
  for(size_t s = 0; s < sntIndices.size(); ++s) {
    int sntStart = sntIndices.at(s);
    if(sntStart == -1) continue; // marked as bad by GetSntIndexes()
    vector<PhrasePair*> phrasePairs;
    ExtractPhrases(sntStart, wrdIndices[s], srcSize, phrasePairs);
    totalPhrases += phrasePairs.size();
    vector<PhrasePair*>::iterator p;
    for (p = phrasePairs.begin(); p != phrasePairs.end(); ++p) {
      assert(*p);
      pair<float, float> lex = GetLexicalWeight(**p);
      pstat_entry entry(TrgPhraseFromSntIdx(**p), Scores(5));
      pair<pstat_iter, bool> foo = pstats.insert(entry);
      Scores& feats = foo.first->second;
      if (foo.second) {
        feats[0]  = 1; // count
        feats[1]  = lex.first;
        feats[3]  = lex.second;
      } else {
        feats[0] += 1;
        feats[1]  = max(feats[1],lex.first);
        feats[3]  = max(feats[3],lex.second);
      }
      delete *p;
    }
  } // done with all sentences
  BOOST_FOREACH(pstat_entry & e, pstats) {
    Scores& feats = e.second;
    // 0: bwd phrase prob
    // 1: lex 1
    // 2: fwd phrase prob
    // 3: lex 2
    // 4: phrase penalty
    float x  = m_trgSA->GetCount(e.first.words)-feats[0] * sampleRate;
    feats[4] = 1;
    feats[3] = log(feats[3]);
    feats[2] = log(feats[0]) - log(totalPhrases);
    feats[1] = log(feats[1]);
    feats[0] = log(feats[0]) - log(feats[0] + x);
  }
  return ret;
}

vector<int>
BilingualDynSuffixArray::
GetSntIndexes(vector<unsigned>& wrdIndices,
              const int sourceSize,
              const vector<unsigned>& sntBreaks) const
{
  vector<unsigned>::const_iterator vit;
  vector<int> sntIndices;
  for(size_t i=0; i < wrdIndices.size(); ++i) {
    vit = upper_bound(sntBreaks.begin(), sntBreaks.end(), wrdIndices[i]);
    int index = int(vit - sntBreaks.begin()) - 1;
    // check for phrases that cross sentence boundaries
    if(wrdIndices[i] - sourceSize + 1 < sntBreaks.at(index))
      sntIndices.push_back(-1);	// set bad flag
    else
      sntIndices.push_back(index);	// store the index of the sentence in the corpus
  }
  return sntIndices;
}

int
BilingualDynSuffixArray::
SampleSelection(vector<unsigned>& sample, int sampleSize) const
{
  // only use top 'sampleSize' number of samples
  vector<unsigned> s;
  randomSample<unsigned>(s,sampleSize,sample.size());
  for (size_t i = 0; i < s.size(); ++i)
    s[i] = sample[s[i]];
  sample.swap(s);
  return sample.size();
}

void
BilingualDynSuffixArray::
addSntPair(string& source, string& target, string& alignment)
{
  vuint_t srcFactor, trgFactor;
  cerr << "source, target, alignment = " << source << ", "
       << target << ", " << alignment << endl;
  const string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
  const unsigned oldSrcCrpSize = m_srcCorpus->size(), oldTrgCrpSize = m_trgCorpus->size();
  cerr << "old source corpus size = " << oldSrcCrpSize << "\told target size = " << oldTrgCrpSize << endl;
  Phrase sphrase(ARRAY_SIZE_INCR);
  sphrase.CreateFromString(Input, m_inputFactors, source, factorDelimiter, NULL);
  m_srcVocab->MakeOpen();
  vector<wordID_t> sIDs(sphrase.GetSize());
  // store words in vocabulary and corpus
  for(int i = sphrase.GetSize()-1; i >= 0; --i) {
    sIDs[i] = m_srcVocab->GetWordID(sphrase.GetWord(i));  // get vocab id backwards
  }
  for(size_t i = 0; i < sphrase.GetSize(); ++i) {
    srcFactor.push_back(sIDs[i]);
    cerr << "srcFactor[" << (srcFactor.size() - 1) << "] = " << srcFactor.back() << endl;
    m_srcCorpus->push_back(srcFactor.back()); // add word to corpus
  }
  m_srcSntBreaks.push_back(oldSrcCrpSize); // former end of corpus is index of new sentence
  m_srcVocab->MakeClosed();
  Phrase tphrase(ARRAY_SIZE_INCR);
  tphrase.CreateFromString(Output, m_outputFactors, target, factorDelimiter, NULL);
  m_trgVocab->MakeOpen();
  vector<wordID_t> tIDs(tphrase.GetSize());
  for(int i = tphrase.GetSize()-1; i >= 0; --i) {
    tIDs[i] = m_trgVocab->GetWordID(tphrase.GetWord(i));  // get vocab id
  }
  for(size_t i = 0; i < tphrase.GetSize(); ++i) {
    trgFactor.push_back(tIDs[i]);
    cerr << "trgFactor[" << (trgFactor.size() - 1) << "] = " << trgFactor.back() << endl;
    m_trgCorpus->push_back(trgFactor.back());
  }
  cerr << "gets to 1\n";
  m_trgSntBreaks.push_back(oldTrgCrpSize);
  cerr << "gets to 2\n";
  m_srcSA->Insert(&srcFactor, oldSrcCrpSize);
  cerr << "gets to 3\n";
  m_trgSA->Insert(&trgFactor, oldTrgCrpSize);
  LoadRawAlignments(alignment);
  m_trgVocab->MakeClosed();

  m_wrd_cooc.Count(sIDs,tIDs, m_rawAlignments.back(),
                   m_srcVocab->GetkOOVWordID(),
                   m_trgVocab->GetkOOVWordID());

  //for(size_t i=0; i < sphrase.GetSize(); ++i)
  //ClearWordInCache(sIDs[i]);

}

void
BilingualDynSuffixArray::
ClearWordInCache(wordID_t srcWord)
{
  if(m_freqWordsCached.find(srcWord) != m_freqWordsCached.end())
    return;
  map<pair<wordID_t, wordID_t>, pair<float, float> >::iterator it,
      first, last;
  for(it = m_wordPairCache.begin(); it != m_wordPairCache.end(); ++it) {
    if(it->first.first == srcWord) {  // all source words grouped
      first = it; // copy first entry of srcWord
      last = it++;
      while(it != m_wordPairCache.end() && (it->first.first == srcWord)) {
        last = it++;
      }
    }
    m_wordPairCache.erase(first, last);
  }
}

SentenceAlignment::
SentenceAlignment(int sntIndex, int sourceSize, int targetSize)
  : m_sntIndex(sntIndex)
  , numberAligned(targetSize, 0)
  , alignedList(sourceSize)
{
  // What is the code below supposed to accomplish??? UG.
  // for(int i=0; i < sourceSize; ++i) {
  //   vector<int> trgWrd;
  //   alignedList[i] = trgWrd;
  // }
}

bool
SentenceAlignment::
Extract(int maxPhraseLength, vector<PhrasePair*> &ret, int startSource, int endSource) const
{
  // foreign = target, F=T
  // english = source, E=S
  int countTarget = numberAligned.size();

  int minTarget = 9999;
  int maxTarget = -1;
  vector< int > usedTarget = numberAligned;
  for(int sourcePos = startSource; sourcePos <= endSource; sourcePos++) {
    for(int ind=0; ind < (int)alignedList[sourcePos].size(); ind++) {
      int targetPos = alignedList[sourcePos][ind];
      // cout << "point (" << targetPos << ", " << sourcePos << ")\n";
      if (targetPos<minTarget) {
        minTarget = targetPos;
      }
      if (targetPos>maxTarget) {
        maxTarget = targetPos;
      }
      usedTarget[ targetPos ]--;
    } // for(int ind=0;ind<sentence
  } // for(int sourcePos=startSource

  // cout << "f projected ( " << minTarget << "-" << maxTarget << ", " << startSource << "," << endSource << ")\n";

  if (maxTarget >= 0 && // aligned to any foreign words at all
      maxTarget-minTarget < maxPhraseLength) {
    // foreign phrase within limits

    // check if foreign words are aligned to out of bound english words
    bool out_of_bounds = false;
    for(int targetPos=minTarget; targetPos <= maxTarget && !out_of_bounds; targetPos++) {
      if (usedTarget[targetPos]>0) {
        // cout << "ouf of bounds: " << targetPos << "\n";
        out_of_bounds = true;
      }
    }

    // cout << "doing if for ( " << minTarget << "-" << maxTarget << ", " << startSource << "," << endSource << ")\n";
    if (!out_of_bounds) {
      // start point of foreign phrase may retreat over unaligned
      for(int startTarget = minTarget;
          (startTarget >= 0 &&
           startTarget > maxTarget-maxPhraseLength && // within length limit
           (startTarget==minTarget || numberAligned[startTarget]==0)); // unaligned
          startTarget--) {
        // end point of foreign phrase may advance over unaligned
        for (int endTarget=maxTarget;
             (endTarget<countTarget &&
              endTarget<startTarget+maxPhraseLength && // within length limit
              (endTarget==maxTarget || numberAligned[endTarget]==0)); // unaligned
             endTarget++) {
          PhrasePair *phrasePair = new PhrasePair(startTarget,endTarget,startSource,endSource, m_sntIndex);
          ret.push_back(phrasePair);
        } // for (int endTarget=maxTarget;
      }	// for(int startTarget=minTarget;
    } // if (!out_of_bounds)
  } // if (maxTarget >= 0 &&
  return (ret.size() > 0);

}

int
BilingualDynSuffixArray::
GetSourceSentenceSize(size_t sentenceId) const
{
  return (sentenceId==m_srcSntBreaks.size()-1) ?
         m_srcCorpus->size() - m_srcSntBreaks.at(sentenceId) :
         m_srcSntBreaks.at(sentenceId+1) - m_srcSntBreaks.at(sentenceId);
}

int
BilingualDynSuffixArray::
GetTargetSentenceSize(size_t sentenceId) const
{
  return (sentenceId==m_trgSntBreaks.size()-1) ?
         m_trgCorpus->size() - m_trgSntBreaks.at(sentenceId) :
         m_trgSntBreaks.at(sentenceId+1) - m_trgSntBreaks.at(sentenceId);
}

BetterPhrase::
BetterPhrase(ScoresComp const& sc)
  : cmp(sc) {}

// bool
// BetterPhrase::
// operator()(pair<Scores, TargetPhrase const*> const& a,
// 	     pair<Scores, TargetPhrase const*> const& b) const
// {
//   return cmp(b.first,a.first);
// }

bool
BetterPhrase::
operator()(pair<Scores, SAPhrase const*> const& a,
           pair<Scores, SAPhrase const*> const& b) const
{
  return cmp(b.first,a.first);
}

}// end namepsace
