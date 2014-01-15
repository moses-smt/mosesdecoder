/* NO OVERLAY VALUES STORED IN SEPERATE FILTER */
#ifndef INC_PERFECTHASH_H
#define INC_PERFECTHASH_H

#include <map>
#include <stdint.h>
#include "hash.h"
#include "RandLMFilter.h"
#include "quantizer.h"

/**
 * PerfectHash handles setting up hash functions and storage
 * for LM data.
 */
using randlm::Filter;
using randlm::BitFilter;
typedef std::map<string, count_t> hpDict_t;
typedef hpDict_t::iterator hpdEntry_t;
static count_t collisions_ = 0;

/* Based on Mortenson et. al. 2006 */
template<typename T>
class PerfectHash
{
public:
  PerfectHash(uint16_t MBs, int width, int bucketRange, float qBase);
  PerfectHash(FileHandler* fin) {
    UTIL_THROW_IF2(fin == 0, "Invalid file handle");
  }
  virtual ~PerfectHash();
  void analyze();
  count_t hpDictMemUse();
  count_t bucketsMemUse();
protected:
  Filter<T>* filter_;
  Filter<T>* values_;
  hpDict_t dict_;
  uint64_t cells_;
  count_t hitMask_;
  int totBuckets_;
  uint8_t bucketRange_;
  uint8_t* idxTracker_;
  uint64_t insert(const wordID_t* IDs, const int len, const count_t value);
  bool update(const wordID_t* IDs, const int len, const count_t value,
              hpdEntry_t& hpdAddr, uint64_t& filterIdx);
  bool update2(const wordID_t* IDs, const int len, const count_t value,
               hpdEntry_t& hpdAddr, uint64_t& filterIdx);
  int query(const wordID_t* IDs, const int len,
            hpdEntry_t& hpdAddr, uint64_t& filterIdx);
  virtual void remove(const wordID_t* IDs, const int len);
  void remove(uint64_t index);
  void save(FileHandler* fout);
  void load(FileHandler* fin);
  virtual void markQueried(const uint64_t&)=0;
  //pointer to a specific entry in a hpDict_t
  virtual void markQueried(hpdEntry_t&)=0;
private:
  T nonZeroSignature(const wordID_t* IDs, const int len, count_t bucket);
  string hpDictKeyValue(const wordID_t* IDs, const int len);
  uint64_t memBound_; // total memory bound in bytes
  uint16_t cellWidth_; // in bits
  UnivHash_linear<count_t>* bucketHash_;
  UnivHash_linear<T>* fingerHash_;
  LogQtizer* qtizer_;
};

template<typename T>
PerfectHash<T>::PerfectHash(uint16_t MBs, int width, int bucketRange,
                            float qBase): hitMask_(1 << 31), memBound_(MBs * (1ULL << 20)),
  cellWidth_(width)
{
  bucketRange_ = static_cast<uint8_t>(bucketRange);
  if(bucketRange > 255) {
    cerr << "ERROR: Max bucket range is > 2^8\n";
    exit(1);
  }
  qtizer_ = new LogQtizer(qBase);
  int valBits = (int)ceil(log2((float)qtizer_->maxcode()));
  cerr << "BITS FOR VALUES ARRAY = " << valBits << endl;
  uint64_t totalBits = memBound_ << 3;
  cells_ = (uint64_t) ceil((float)totalBits / (float)(cellWidth_ + valBits)); // upper bound on cells
  cells_ += (cells_ % bucketRange_); // make cells multiple of bucket range
  totBuckets_ = (cells_ / bucketRange_) - 1; // minus 1 so totBuckets * bucksize + bucksize = cells
  filter_ = new Filter<T>(cells_, cellWidth_);
  values_ = new Filter<T>(cells_, valBits);
  idxTracker_ = new uint8_t[totBuckets_];
  for(int i=0; i < totBuckets_; ++i) idxTracker_[i] = 0;
  // initialize ranges for each hash function
  bucketHash_ = new UnivHash_linear<count_t>(totBuckets_, 1, PRIME);
  fingerHash_ = new UnivHash_linear<T>(pow(2.0f, cellWidth_), MAX_HASH_FUNCS, PRIME);
}

template<typename T>
PerfectHash<T>::~PerfectHash()
{
  delete[] idxTracker_;
  delete filter_;
  filter_ = NULL;
  delete fingerHash_;
  delete bucketHash_;
  delete qtizer_;
  delete values_;
}

template<typename T>
uint64_t PerfectHash<T>::insert(const wordID_t* IDs, const int len,
                                const count_t value)
{
  count_t bucket = (bucketHash_->size() > 1 ? bucketHash_->hash(IDs, len, len) : bucketHash_->hash(IDs, len, 0));
  if(idxTracker_[bucket] < (int)bucketRange_) {  // if empty rows
    // restriction on fprint value is non-zero
    T fp = nonZeroSignature(IDs, len, (bucket % MAX_HASH_FUNCS));
    uint64_t emptyidx = cells_ + 1;
    uint64_t index = bucket * bucketRange_,  // starting bucket row
             lastrow = index + bucketRange_;  // ending row
    while(index < lastrow) { // unique so check each row for "matching" signature
      T filterVal = filter_->read(index);
      if((filterVal == 0) && (emptyidx == cells_ + 1)) {  // record first empty row
        emptyidx = index;
      } else if(filterVal == fp) {
        ++collisions_;
        dict_[hpDictKeyValue(IDs, len)] = value; // store exact in hpd
        return cells_ + 1;  // finished
      }
      ++index;
    }
    UTIL_THROW_IF2((emptyidx >= index) || (filter_->read(emptyidx) != 0), "Error"); // should have found empty index if it gets here
    T code = (T)qtizer_->code(value);
    filter_->write(emptyidx, fp); // insert the fprint
    values_->write(emptyidx, code);
    ++idxTracker_[bucket]; // keep track of bucket size
    return emptyidx;
  } else { // bucket is full
    dict_[hpDictKeyValue(IDs, len)] = value; // add to hpd
    return cells_ + 1;
  }
}

template<typename T>
bool PerfectHash<T>::update(const wordID_t* IDs, const int len,
                            const count_t value, hpdEntry_t& hpdAddr, uint64_t& filterIdx)
{
  // check if key is in high perf. dictionary
  filterIdx = cells_ + 1;
  string skey = hpDictKeyValue(IDs, len);
  if((hpdAddr = dict_.find(skey)) != dict_.end()) {
    hpdAddr->second = value;
    return true;
  }
  // else hash ngram
  //count_t bucket = bucketHash_->hash(IDs, len);
  count_t bucket = (bucketHash_->size() > 1 ? bucketHash_->hash(IDs, len, len) : bucketHash_->hash(IDs, len, 0));
  // restriction on fprint value is non-zero
  T fp = nonZeroSignature(IDs, len, (bucket % MAX_HASH_FUNCS));
  uint64_t index = bucket * bucketRange_,  // starting bucket row
           lastrow = index + bucketRange_;
  while(index < lastrow) { // must check each row for matching fp event
    T filterVal = filter_->read(index);
    if(filterVal == fp) { // found event w.h.p.
      values_->write(index, (T)qtizer_->code(value));
      filterIdx = index;
      return true;
    }
    ++index;
  }
  // could add if it gets here.
  return false;
}

template<typename T>
int PerfectHash<T>::query(const wordID_t* IDs, const int len,
                          hpdEntry_t& hpdAddr, uint64_t& filterIdx)
{
  // check if key is in high perf. dictionary
  string skey = hpDictKeyValue(IDs, len);
  if((hpdAddr = dict_.find(skey)) != dict_.end()) {
    filterIdx = cells_ + 1;
    return(hpdAddr->second);  // returns copy of value
  } else { // check if key is in filter
    // get bucket
    //count_t bucket = bucketHash_->hash(IDs, len);
    count_t bucket = (bucketHash_->size() > 1 ? bucketHash_->hash(IDs, len, len) : bucketHash_->hash(IDs, len, 0));
    // restriction on fprint value is non-zero
    T fp = nonZeroSignature(IDs, len, (bucket % MAX_HASH_FUNCS));
    // return value if ngram is in filter
    uint64_t index = bucket * bucketRange_,
             lastrow = index + bucketRange_;
    for(; index < lastrow; ++index) {
      if(filter_->read(index) == fp) {
        //cout << "fp = " << fp << "\tbucket = " << bucket << "\tfilter =" <<
        //filter_->read(index) << "\tcode = " << code << endl;
        filterIdx = index;
        hpdAddr = dict_.end();
        return (int)qtizer_->value(values_->read(index));
      }
    }
  }
  return -1;
}

template<typename T>
void PerfectHash<T>::remove(const wordID_t* IDs, const int len)
{
  // delete key if in high perf. dictionary
  string skey = hpDictKeyValue(IDs, len);
  if(dict_.find(skey) != dict_.end())
    dict_.erase(skey);
  else {  // check if key is in filter
    // get small representation for ngrams
    //count_t bucket = bucketHash_->hash(IDs, len);
    count_t bucket = (bucketHash_->size() > 1? bucketHash_->hash(IDs, len, len) : bucketHash_->hash(IDs, len, 0));
    // retrieve non zero fingerprint for ngram
    T fp = nonZeroSignature(IDs, len, (bucket % MAX_HASH_FUNCS));
    // return value if ngram is in filter
    uint64_t index = bucket * bucketRange_,
             lastrow = index + bucketRange_;
    for(; index < lastrow; ++index) {
      if(filter_->read(index) == fp) {
        filter_->write(index, 0);
        values_->write(index, 0);
        --idxTracker_[bucket]; // track bucket size reduction
        break;
      }
    }
  }
}

template<typename T> // clear filter index
void PerfectHash<T>::remove(uint64_t index)
{
  UTIL_THROW_IF2(index >= cells_, "Out of bound: " << index);
  UTIL_THROW_IF2(filter_->read(index) == 0, "Error"); // slow
  filter_->write(index, 0);
  values_->write(index, 0);
  //reduce bucket size
  count_t bucket = index / bucketRange_;
  --idxTracker_[bucket];
}

template<typename T>
T PerfectHash<T>::nonZeroSignature(const wordID_t* IDs, const int len,
                                   count_t bucket)
{
  count_t h = bucket;
  T fingerprint(0);
  do {
    fingerprint = fingerHash_->hash(IDs, len, h);
    h += (h < fingerHash_->size() - 1 ? 1 : -h); // wrap around
  } while((fingerprint == 0) && (h != bucket));
  if(fingerprint == 0)
    cerr << "WARNING: Unable to find non-zero signature for ngram\n" << endl;
  return fingerprint;
}

template<typename T>
string PerfectHash<T>::hpDictKeyValue(const wordID_t* IDs, const int len)
{
  string skey(" ");
  for(int i = 0; i < len; ++i)
    skey += Utils::IntToStr(IDs[i]) + "¬";
  Utils::trim(skey);
  return skey;
}

template<typename T>
count_t PerfectHash<T>::hpDictMemUse()
{
  // return hpDict memory usage in MBs
  return (count_t) sizeof(hpDict_t::value_type)* dict_.size() >> 20;
}

template<typename T>
count_t PerfectHash<T>::bucketsMemUse()
{
  // return bucket memory usage in MBs
  return (count_t) (filter_->size() + values_->size());
}

template<typename T>
void PerfectHash<T>::save(FileHandler* fout)
{
  UTIL_THROW_IF2(fout == 0, "Invalid file handle");
  cerr << "\tSaving perfect hash parameters...\n";
  fout->write((char*)&hitMask_, sizeof(hitMask_));
  fout->write((char*)&memBound_, sizeof(memBound_));
  fout->write((char*)&cellWidth_, sizeof(cellWidth_));
  fout->write((char*)&cells_, sizeof(cells_));
  fout->write((char*)&totBuckets_, sizeof(totBuckets_));
  fout->write((char*)&bucketRange_, sizeof(bucketRange_));
  fout->write((char*)idxTracker_, totBuckets_ * sizeof(idxTracker_[0]));
  qtizer_->save(fout);
  cerr << "\tSaving hash functions...\n";
  fingerHash_->save(fout);
  bucketHash_->save(fout);
  cerr << "\tSaving bit filter...\n";
  filter_->save(fout);
  values_->save(fout);
  cerr << "\tSaving high performance dictionary...\n";
  count_t size = dict_.size();
  fout->write((char*)&size, sizeof(count_t));
  *fout << endl;
  iterate(dict_, t)
  *fout << t->first << "\t" << t->second << "\n";
}

template<typename T>
void PerfectHash<T>::load(FileHandler* fin)
{
  UTIL_THROW_IF2(fin == 0, "Invalid file handle");
  cerr << "\tLoading perfect hash parameters...\n";
  fin->read((char*)&hitMask_, sizeof(hitMask_));
  fin->read((char*)&memBound_, sizeof(memBound_));
  fin->read((char*)&cellWidth_, sizeof(cellWidth_));
  fin->read((char*)&cells_, sizeof(cells_));
  fin->read((char*)&totBuckets_, sizeof(totBuckets_));
  fin->read((char*)&bucketRange_, sizeof(bucketRange_));
  idxTracker_ = new uint8_t[totBuckets_];
  fin->read((char*)idxTracker_, totBuckets_ * sizeof(idxTracker_[0]));
  qtizer_ = new LogQtizer(fin);
  cerr << "\tLoading hash functions...\n";
  fingerHash_ = new UnivHash_linear<T>(fin);
  bucketHash_ = new UnivHash_linear<count_t>(fin);
  cerr << "\tLoading bit filter...\n";
  filter_ = new Filter<T>(fin);
  values_ = new Filter<T>(fin);
  cerr << "\tLoading HPD...\n";
  count_t size = 0;
  fin->read((char*)&size, sizeof(count_t));
  fin->ignore(256, '\n');
  string line;
  hpDict_t::key_type key;
  hpDict_t::mapped_type val;
  for(count_t i=0; i < size; ++i) {
    getline(*fin, line);
    Utils::trim(line);
    std::istringstream ss(line.c_str());
    ss >> key, ss >> val;
    dict_[key] = val;
  }
  cerr << "\tHPD size=" << dict_.size() << endl;
  cerr << "Finished loading ORLM." << endl;
}

template<typename T>
void PerfectHash<T>::analyze()
{
  cerr << "Analyzing Dynamic Bloomier Filter...\n";
  // see how many items in each bucket
  uint8_t* bucketCnt = new uint8_t[totBuckets_];
  unsigned largestBucket = 0, totalCellsSet = 0,
           smallestBucket = bucketRange_, totalZeroes = 0;
  int curBucket = -1, fullBuckets(0);
  for(int i = 0; i < totBuckets_; ++i) bucketCnt[i] = 0;
  for(uint64_t i =0; i < cells_; ++i) {
    if(i % bucketRange_ == 0) ++curBucket;
    if(filter_->read(i) != 0) {
      ++bucketCnt[curBucket];
      ++totalCellsSet;
    } else ++totalZeroes;
  }
  count_t bi = 0, si = 0;
  for(int i = 0; i < totBuckets_; ++i) {
    if(bucketCnt[i] > largestBucket) {
      largestBucket = bucketCnt[i];
      bi = i;
    } else if(bucketCnt[i] < smallestBucket) {
      smallestBucket = bucketCnt[i];
      si = i;
    }
  }
  count_t trackerCells(0);
  for(int i = 0; i < totBuckets_; i++) {
    trackerCells += idxTracker_[i];
    if(idxTracker_[i] == bucketRange_)
      ++fullBuckets;
  }
  for(int i = 0; i < totBuckets_; ++i) {
    if(bucketCnt[i] != idxTracker_[i])
      cerr << "bucketCnt[" << i << "] = " << (int)bucketCnt[i] <<
           "\tidxTracker_[" << i << "] = " << (int)idxTracker_[i] << endl;
  }
  cerr << "total cells= " << cells_ << endl;
  cerr << "total buckets= " << totBuckets_ << endl;
  cerr << "bucket range= " << (int)bucketRange_ << endl;
  cerr << "fingerprint bits= " << cellWidth_ << endl;
  cerr << "total cells set= " << totalCellsSet;
  cerr << " (idxTracker set = " << trackerCells << ")" << endl;
  cerr << "total zeroes=" << totalZeroes;
  cerr << " (idxTracker zeros = " << cells_ - trackerCells << ")" << endl;
  cerr << "largest bucket (" << bi << ") size= " << largestBucket << endl;
  cerr << "smallest bucket (" << si << ") size= " << smallestBucket << endl;
  cerr << "last bucket size= " << (int)bucketCnt[totBuckets_ - 1] <<
       " (idxTracker last bucket size = " << (int)idxTracker_[totBuckets_ - 1] << ")" << endl;
  cerr << "total buckets full = " << fullBuckets << endl;
  cerr << "total collision errors= " << collisions_ << endl;
  cerr << "high performance dictionary size= " << dict_.size() << endl;
  cerr << "high performance dictionary MBs= " << hpDictMemUse() << endl;
  cerr << "filter MBs= " << filter_->size() << endl;
  cerr << "values MBs= " << values_->size() << endl;
  delete[] bucketCnt;
}

template<typename T>
bool PerfectHash<T>::update2(const wordID_t* IDs, const int len,
                             const count_t value, hpdEntry_t& hpdAddr, uint64_t& filterIdx)
{
  // check if key is in high perf. dictionary
  filterIdx = cells_ + 1;
  string skey = hpDictKeyValue(IDs, len);
  if((hpdAddr = dict_.find(skey)) != dict_.end()) {
    hpdAddr->second += value;
    return true;
  }
  // else hash ngram
  //count_t bucket = bucketHash_->hash(IDs, len);
  count_t bucket = (bucketHash_->size() > 1 ? bucketHash_->hash(IDs, len, len) : bucketHash_->hash(IDs, len, 0));
  // restriction on fprint value is non-zero
  T fp = nonZeroSignature(IDs, len, (bucket % MAX_HASH_FUNCS));
  uint64_t index = bucket * bucketRange_,  // starting bucket row
           lastrow = index + bucketRange_;
  while(index < lastrow) { // must check each row for matching fp event
    T filterVal = filter_->read(index);
    if(filterVal == fp) { // found event w.h.p.
      int oldval = (int)qtizer_->value(values_->read(index));
      values_->write(index, (T)qtizer_->code(oldval + value));
      filterIdx = index;
      return true;
    }
    ++index;
  }
  // add if it gets here.
  insert(IDs, len, value);
  return false;
}

#endif

