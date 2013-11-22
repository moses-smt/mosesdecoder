#ifndef INC_DYNAMICLM_H
#define INC_DYNAMICLM_H

#include <algorithm>
#include <vector>
#include "perfectHash.h"
#include "RandLMCache.h"
#include "types.h"
#include "vocab.h"

/*
 * DynamicLM manipulates LM
 */
using randlm::BitFilter;
using randlm::Cache;

const bool strict_checks_ = false;

//! @todo ask abby2
template<typename T>
class OnlineRLM: public PerfectHash<T>
{
public:
  OnlineRLM(uint16_t MBs, int width, int bucketRange, count_t order,
            Moses::Vocab* v, float qBase = 8): PerfectHash<T>(MBs, width, bucketRange, qBase),
    vocab_(v), bAdapting_(false), order_(order), corpusSize_(0), alpha_(0) {
    UTIL_THROW_IF2(vocab_ == 0, "Vocab object not set");
    //instantiate quantizer class here
    cache_ = new Cache<float>(8888.8888, 9999.9999); // unknown_value, null_value
    alpha_ = new float[order_ + 1];
    for(count_t i = 0; i <= order_; ++i)
      alpha_[i] = i * log10(0.4);
    cerr << "Initialzing auxillary bit filters...\n";
    bPrefix_ = new BitFilter(this->cells_);
    bHit_ = new BitFilter(this->cells_);
  }
  OnlineRLM(FileHandler* fin, count_t order):
    PerfectHash<T>(fin), bAdapting_(true), order_(order), corpusSize_(0) {
    load(fin);
    cache_ = new Cache<float>(8888.8888, 9999.9999); // unknown_value, null_value
    alpha_ = new float[order_ + 1];
    for(count_t i = 0; i <= order_; ++i)
      alpha_[i] = i * log10(0.4);
  }
  ~OnlineRLM() {
    if(alpha_) delete[] alpha_;
    if(bAdapting_) delete vocab_;
    else vocab_ = NULL;
    if(cache_) delete cache_;
    delete bPrefix_;
    delete bHit_;
  }
  float getProb(const wordID_t* ngram, int len, const void** state);
  //float getProb2(const wordID_t* ngram, int len, const void** state);
  bool insert(const std::vector<string>& ngram, const int value);
  bool update(const std::vector<string>& ngram, const int value);
  int query(const wordID_t* IDs, const int len);
  int sbsqQuery(const std::vector<string>& ngram, int* len,
                bool bStrict = false);
  int sbsqQuery(const wordID_t* IDs, const int len, int* codes,
                bool bStrict = false);
  void remove(const std::vector<string>& ngram);
  count_t heurDelete(count_t num2del, count_t order = 5);
  uint64_t corpusSize() {
    return corpusSize_;
  }
  void corpusSize(uint64_t c) {
    corpusSize_ = c;
  }
  void clearCache() {
    if(cache_) cache_->clear();
  }
  void save(FileHandler* fout);
  void load(FileHandler* fin);
  void randDelete(int num2del);
  int countHits();
  int countPrefixes();
  int cleanUpHPD();
  void clearMarkings();
  void removeNonMarked();
  Moses::Vocab* vocab_;
protected:
  void markQueried(const uint64_t& index);
  void markQueried(hpdEntry_t& value);
  bool markPrefix(const wordID_t* IDs, const int len, bool bSet);
private:
  const void* getContext(const wordID_t* ngram, int len);
  const bool bAdapting_; // used to signal adaptation of model
  const count_t order_; // LM order
  uint64_t corpusSize_; // total training corpus size
  float* alpha_;  // backoff constant
  Cache<float>* cache_;
  BitFilter* bPrefix_;
  BitFilter* bHit_;
};

template<typename T>
bool OnlineRLM<T>::insert(const std::vector<string>& ngram, const int value)
{
  int len = ngram.size();
  wordID_t wrdIDs[len];
  uint64_t index(this->cells_ + 1);
  for(int i = 0; i < len; ++i)
    wrdIDs[i] = vocab_->GetWordID(ngram[i]);
  index = PerfectHash<T>::insert(wrdIDs, len, value);
  if(value > 1 && len < order_)
    markPrefix(wrdIDs, ngram.size(), true); // mark context
  // keep track of total items from training data minus "<s>"
  if(ngram.size() == 1 && (!bAdapting_)) // hack to not change corpusSize when adapting
    corpusSize_ += (wrdIDs[0] != vocab_->GetBOSWordID()) ? value : 0;
  if(bAdapting_ && (index < this->cells_)) // mark to keep while adapting
    markQueried(index);
  return true;
}

template<typename T>
bool OnlineRLM<T>::update(const std::vector<string>& ngram, const int value)
{
  int len = ngram.size();
  std::vector<wordID_t> wrdIDs(len);
  uint64_t index(this->cells_ + 1);
  hpdEntry_t hpdItr;
  vocab_->MakeOpen();
  for(int i = 0; i < len; ++i)
    wrdIDs[i] = vocab_->GetWordID(ngram[i]);
  // if updating, minimize false positives by pre-checking if context already in model
  bool bIncluded(true);
  if(value > 1 && len < (int)order_)
    bIncluded = markPrefix(&wrdIDs[0], ngram.size(), true); // mark context
  if(bIncluded) { // if context found
    bIncluded = PerfectHash<T>::update2(&wrdIDs[0], len, value, hpdItr, index);
    if(index < this->cells_) {
      markQueried(index);
    } else if(hpdItr != this->dict_.end()) markQueried(hpdItr);
  }

  return bIncluded;
}
template<typename T>
int OnlineRLM<T>::query(const wordID_t* IDs, int len)
{
  uint64_t filterIdx = 0;
  hpdEntry_t hpdItr;
  int value(0);
  value = PerfectHash<T>::query(IDs, len, hpdItr, filterIdx);
  if(value != -1) {
    if(hpdItr != this->dict_.end()) {
      //markQueried(hpdItr);  // mark this event as "hit"
      value -= ((value & this->hitMask_) != 0) ? this->hitMask_ : 0; // check for previous hit marks
    } else {
    	UTIL_THROW_IF2(filterIdx >= this->cells_,
    			"Out of bound: " << filterIdx);
      //markQueried(filterIdx);
    }
  }
  return value > 0 ? value : 0;
}

template<typename T>
bool OnlineRLM<T>::markPrefix(const wordID_t* IDs, const int len, bool bSet)
{
  if(len <= 1) return true; // only do this for for ngrams with context
  static Cache<int> pfCache(-1, -1); // local prefix cache
  int code(0);
  if(!pfCache.checkCacheNgram(IDs, len - 1, &code, NULL)) {
    hpdEntry_t hpdItr;
    uint64_t filterIndex(0);
    code = PerfectHash<T>::query(IDs, len - 1, hpdItr, filterIndex); // hash IDs[0..len-1]
    if(code == -1) { // encountered false positive in pipeline
      cerr << "WARNING: markPrefix(). The O-RLM is *not* well-formed.\n";
      // add all prefixes or return false;
      return false;
    }
    if(filterIndex != this->cells_ + 1) {
      UTIL_THROW_IF2(hpdItr != this->dict_.end(), "Error");
      if(bSet) bPrefix_->setBit(filterIndex); // mark index
      else bPrefix_->clearBit(filterIndex);   // unset index
    } else {
      UTIL_THROW_IF2(filterIndex != this->cells_ + 1, "Error");
      //how to handle hpd prefixes?
    }
    if(pfCache.nodes() > 10000) pfCache.clear();
    pfCache.setCacheNgram(IDs, len - 1, code, NULL);
  }
  return true;
}

template<typename T>
void OnlineRLM<T>::markQueried(const uint64_t& index)
{
  bHit_->setBit(index);
  //cerr << "filter[" << index << "] = " << this->filter_->read(index) << endl;
}

template<typename T>
void OnlineRLM<T>::markQueried(hpdEntry_t& value)
{
  // set high bit of counter to indicate "hit" status
  value->second |= this->hitMask_;
}

template<typename T>
void OnlineRLM<T>::remove(const std::vector<string>& ngram)
{
  wordID_t IDs[ngram.size()];
  for(count_t i = 0; i < ngram.size(); ++i)
    IDs[i] = vocab_->GetWordID(ngram[i]);
  PerfectHash<T>::remove(IDs, ngram.size());
}

template<typename T>
count_t OnlineRLM<T>::heurDelete(count_t num2del, count_t order)
{
  count_t deleted = 0;
  cout << "Deleting " << num2del << " of order "<< order << endl;
  // delete from filter first
  int full = *std::max_element(this->idxTracker_, this->idxTracker_
                               + this->totBuckets_);
  for(; full > 0; --full) // delete from fullest buckets first
    for(int bk = 0; bk < this->totBuckets_; ++bk) {
      if(deleted >= num2del) break;
      if(this->idxTracker_[bk] == full) {        // if full
        uint64_t first = bk * this->bucketRange_,
                 last = first + this->bucketRange_;
        for(uint64_t row = first; row < last; ++row) {  // check each row
          if(!(bHit_->testBit(row) || bPrefix_->testBit(row) )) {
            if(this->filter_->read(row) != 0) {
              PerfectHash<T>::remove(row);  // remove from filter
              ++deleted;
            }
          }
        }
      }
    }
  if(deleted < num2del) {
    // remove from hpd
    cerr << "TODO! HPD deletions\n";
  }
  cerr << "Total deleted = " << deleted << endl;
  return deleted;
}

template<typename T>
int OnlineRLM<T>::sbsqQuery(const std::vector<string>& ngram, int* codes,
                            bool bStrict)
{
  wordID_t IDs[ngram.size()];
  for(count_t i = 0; i < ngram.size(); ++i)
    IDs[i] = vocab_->GetWordID(ngram[i]);
  return sbsqQuery(IDs, ngram.size(), codes, bStrict);
}

template<typename T>
int OnlineRLM<T>::sbsqQuery(const wordID_t* IDs, const int len, int* codes,
                            bool bStrict)
{
  uint64_t filterIdx = 0;
  int val(0), fnd(0);
  hpdEntry_t hpdItr;
  for(int i = len - 1; i >= 0; --i) { // do subsequence filtering
    //if(IDs[i] == Vocab::kOOVWordID) break;
    val = PerfectHash<T>::query(&IDs[i], len - i, hpdItr, filterIdx);
    if(val != -1) { // if event found
      fnd = len - i; // increment found sequence
      if(hpdItr != this->dict_.end()) {
        val -= ((val & this->hitMask_) != 0) ? this->hitMask_ : 0; // account for previous hit marks
      }
    } else if(bStrict) {
      break;
    }
    // add to value array
    codes[i] = val > 0 ? val : 0;
  }
  while(bStrict && (fnd > 1)) { // do checks the other way
    val = PerfectHash<T>::query(&IDs[len - fnd], fnd - 1, hpdItr, filterIdx);
    if(val != -1) break; // if anything found
    else --fnd; // else decrement found
  }

  return fnd;
}

template<typename T>
float OnlineRLM<T>::getProb(const wordID_t* ngram, int len,
                            const void** state)
{
  static const float oovprob = log10(1.0 / (static_cast<float>(vocab_->Size()) - 1));
  float logprob(0);
  const void* context = (state) ? *state : 0;
  // if full ngram and prob not in cache
  if(!cache_->checkCacheNgram(ngram, len, &logprob, &context)) {
    // get full prob and put in cache
    int num_fnd(0), den_val(0);
    int *in = new int[len]; // in[] keeps counts of increasing order numerator
    for(int i = 0; i < len; ++i) in[i] = 0;
    for(int i = len - 1; i >= 0; --i) {
      if(ngram[i] == vocab_->GetkOOVWordID()) break;  // no need to query if OOV
      in[i] = query(&ngram[i], len - i);
      if(in[i] > 0) {
        num_fnd = len - i;
      } else if(strict_checks_) break;
    }
    while(num_fnd > 1) { // get lower order count
      //get sub-context of size one less than length found (exluding target)
      if(((den_val = query(&ngram[len - num_fnd], num_fnd - 1)) > 0) &&
          (den_val >= in[len - num_fnd]) && (in[len - num_fnd] > 0)) {
        break;
      } else --num_fnd; // else backoff to lower ngram order
    }
    if(num_fnd == 1 && (in[len - 1] < 1)) // sanity check for unigrams
      num_fnd = 0;
    switch(num_fnd) { // find prob (need to refactor into precomputation)
    case 0: // OOV
      logprob = alpha_[len] + oovprob;
      break;
    case 1: // unigram found only
      UTIL_THROW_IF2(in[len - 1] <= 0, "Error");
      logprob = alpha_[len - 1] + (corpusSize_ > 0 ?
                                   log10(static_cast<float>(in[len - 1]) / static_cast<float>(corpusSize_)) : 0);
      //logprob = alpha_[len - 1] +
      //log10(static_cast<float>(in[len - 1]) / static_cast<float>(corpusSize_));
      break;
    default:
      UTIL_THROW_IF2(den_val <= 0, "Error");
      //if(subgram == in[len - found]) ++subgram; // avoid returning zero probs????
      logprob = alpha_[len - num_fnd] +
                log10(static_cast<float>(in[len - num_fnd]) / static_cast<float>(den_val));
      break;
    }
    // need unique context
    context = getContext(&ngram[len - num_fnd], num_fnd);
    // put whatever was found in cache
    cache_->setCacheNgram(ngram, len, logprob, context);
  } // end checkCache
  return logprob;
}

template<typename T>
const void* OnlineRLM<T>::getContext(const wordID_t* ngram, int len)
{
  int dummy(0);
  float**addresses = new float*[len];  // only interested in addresses of cache
  UTIL_THROW_IF2(cache_->getCache2(ngram, len, &addresses[0], &dummy) != len,
		  "Error");
  // return address of cache node

  float *addr0 = addresses[0];
  free( addresses );
  return (const void*)addr0;
}

template<typename T>
void OnlineRLM<T>::randDelete(int num2del)
{
  int deleted = 0;
  for(uint64_t i = 0; i < this->cells_; i++) {
    if(this->filter_->read(i) != 0) {
      PerfectHash<T>::remove(i);
      ++deleted;
    }
    if(deleted >= num2del) break;
  }
}

template<typename T>
int OnlineRLM<T>::countHits()
{
  int hit(0);
  for(uint64_t i = 0; i < this->cells_; ++i)
    if(bHit_->testBit(i)) ++hit;
  iterate(this->dict_, itr)
  if((itr->second & this->hitMask_) != 0)
    ++hit;
  cerr << "Hit count = " << hit << endl;
  return hit;
}

template<typename T>
int OnlineRLM<T>::countPrefixes()
{
  int pfx(0);
  for(uint64_t i = 0; i < this->cells_; ++i)
    if(bPrefix_->testBit(i)) ++pfx;
  //TODO::Handle hpdict prefix counts
  cerr << "Prefix count (in filter) = " << pfx << endl;
  return pfx;
}

template<typename T>
int OnlineRLM<T>::cleanUpHPD()
{
  cerr << "HPD size before = " << this->dict_.size() << endl;
  std::vector<string> vDel, vtmp;
  iterate(this->dict_, itr) {
    if(((itr->second & this->hitMask_) == 0) &&  // if not hit during testing
        (Utils::splitToStr(itr->first, vtmp, "¬") >= 3)) {  // and higher order ngram
      vDel.push_back(itr->first);
    }
  }
  iterate(vDel, vitr)
  this->dict_.erase(*vitr);
  cerr << "HPD size after = " << this->dict_.size() << endl;
  return vDel.size();
}

template<typename T>
void OnlineRLM<T>::clearMarkings()
{
  cerr << "clearing all event hits\n";
  bHit_->reset();
  count_t* value(0);
  iterate(this->dict_, itr) {
    value = &itr->second;
    *value -= ((*value & this->hitMask_) != 0) ? this->hitMask_ : 0;
  }
}

template<typename T>
void OnlineRLM<T>::save(FileHandler* fout)
{
  cerr << "Saving ORLM...\n";
  // save vocab
  vocab_->Save(fout);
  fout->write((char*)&corpusSize_, sizeof(corpusSize_));
  fout->write((char*)&order_, sizeof(order_));
  bPrefix_->save(fout);
  bHit_->save(fout);
  // save everything else
  PerfectHash<T>::save(fout);
  cerr << "Finished saving ORLM." << endl;
}

template<typename T>
void OnlineRLM<T>::load(FileHandler* fin)
{
  cerr << "Loading ORLM...\n";
  // load vocab first
  vocab_ = new Moses::Vocab(fin);
  UTIL_THROW_IF2(vocab_ == 0, "Vocab object not set");
  fin->read((char*)&corpusSize_, sizeof(corpusSize_));
  cerr << "\tCorpus size = " << corpusSize_ << endl;
  fin->read((char*)&order_, sizeof(order_));
  cerr << "\tModel order = " << order_ << endl;
  bPrefix_ = new BitFilter(fin);
  bHit_ = new BitFilter(fin);
  // load everything else
  PerfectHash<T>::load(fin);
}

template<typename T>
void OnlineRLM<T>::removeNonMarked()
{
  cerr << "deleting all unused events\n";
  int deleted(0);
  for(uint64_t i = 0; i < this->cells_; ++i) {
    if(!(bHit_->testBit(i) || bPrefix_->testBit(i))
        && (this->filter_->read(i) != 0)) {
      PerfectHash<T>::remove(i);
      ++deleted;
    }
  }
  deleted += cleanUpHPD();
  cerr << "total removed from ORLM = " << deleted << endl;
}

/*
template<typename T>
float OnlineRLM<T>::getProb2(const wordID_t* ngram, int len, const void** state) {
  static const float oovprob = log10(1.0 / (static_cast<float>(vocab_->size()) - 1));
  float log_prob(0);
  const void* context_state(NULL);
  int found;
  int* denom_codes[order_];
  int* num_codes[order_ + 1];
  int denom_found(0);
  cerr << "length=" << len << endl;
  // constrain cache queries using model assumptions
  int denom_len = cache_->getCache(ngram, len - 1, &denom_codes[0], &denom_found);
  cerr << "denom_len = " << denom_len << endl;
  int num_len = cache_->getCache(&ngram[len - denom_len - 1], denom_len + 1,
                                          &num_codes[0], &found);
  cerr << "num_len= " << num_len << endl;
  // keed reducing ngram size until both denominator and numerator are found
  // allowed to leave kUnknownCode in cache because we check for this.
  found = num_len; // guaranteed to be <= denom_len + 1
  // still check for OOV
  for (int i = len - found; i < len; ++i)
    if (ngram[i] == Vocab::kOOVWordID) {
        found = len - i - 1;
    }
  // check for relative estimator
  while(found > 1) {
    if(*denom_codes[found-1] == cache_unk_ &&
      ((*denom_codes[found-1] = query(&ngram[len-found], found-1)) == 0)) {
        //!struct_->query(&ngram[len-*found], *found-1, kMainEventIdx, denom_codes[*found-1])) {
      *num_codes[found] = cache_unk_;
    } else {
      if(*num_codes[found] != cache_unk_ ||
        ((*num_codes[found] = query(&ngram[len-found], found)) <= *denom_codes[found-1]))
        //  struct_->query(&ngram[len-*found], *found, kMainEventIdx,
        //                 num_codes[*found], *denom_codes[*found-1]))
        break;
    }
    --found;
  }
  // didn't find bigram numerator or unigram denominator
  if (found == 1)
    found = *num_codes[1] != cache_unk_
      || ((*num_codes[1] = query(&ngram[len - 1], 1)) != 0);
      //struct_->query(&ngram[len - 1], 1, kMainEventIdx, num_codes[1]);
  // ....
  // return estimate applying correct backoff score (precomputed)
  // store full log prob with complete ngram (even if backed off)
  switch (found) {
  case 0: // no observation: assign prob of 'new word' in training data
    log_prob = alpha_[len] + oovprob;
    //log_prob = stupid_backoff_log10_[len] + uniform_log10prob_;
    break;
  case 1: // unigram over whole corpus
    log_prob = alpha_[len - 1] +
      log10(static_cast<float>(*num_codes[1]) / static_cast<float>(corpusSize_));
    //log_prob = log_quantiser_->getLog10Value(*num_codes[1]) - corpus_size_log10_
    //  + stupid_backoff_log10_[len - 1];  // precomputed
    break;
  default: // otherwise use both statistics and (possibly zero) backoff weight
    log_prob = alpha_[len - found] +
      log10(static_cast<float>(*num_codes[found]) / static_cast<float>(*denom_codes[found-1]));
    //log_prob = log_quantiser_->getLog10Value(*num_codes[*found ])
    //  - log_quantiser_->getLog10Value(*denom_codes[*found - 1])
    //  + stupid_backoff_log10_[len - *found];
  }
  context_state = (const void*)num_codes[found == len ? found - 1 : found];;
  //probCache_->store(len, log_prob, context_state);
  if (state)
    *state = context_state;
  return log_prob;
}
*/

#endif

