#ifndef INC_ALLHASHFUNCS_H
#define INC_ALLHASHFUNCS_H

#include <cmath>
#include "types.h"
#include "utils.h"
#include "FileHandler.h"
#include "util/exception.hh"

using namespace Moses;
typedef uint64_t P;   // largest input range is 2^64

//! @todo ask abby2
template <typename T>
class HashBase
{
protected:
  T m_;       // range of hash output
  count_t H_; // number of hash functions to instantiate
  virtual void initSeeds()=0;
  virtual void freeSeeds()=0;
public:
  HashBase(float m, count_t H=1):m_((T)m), H_(H) {
    //cerr << "range = (0..." << m_ << "]" << endl;
  }
  HashBase(FileHandler* fin) {
    load(fin);
  }
  virtual ~HashBase() {}
  virtual T hash(const char*s, count_t h)=0;  // string hashing
  virtual T hash(const wordID_t* id, const int len, count_t h)=0;  // vocab mapped hashing
  count_t size() {
    return H_;
  }
  virtual void save(FileHandler* fout) {
    UTIL_THROW_IF2(fout == 0, "Null file handle");
    fout->write((char*)&m_, sizeof(m_));
    fout->write((char*)&H_, sizeof(H_));
  }
  virtual void load(FileHandler* fin) {
    UTIL_THROW_IF2(fin == 0, "Null file handle");
    fin->read((char*)&m_, sizeof(m_));
    fin->read((char*)&H_, sizeof(H_));
  }
};

//! @todo ask abby2
template <typename T>
class UnivHash_linear: public HashBase<T>
{
public:
  UnivHash_linear(float m, count_t H, P pr):
    HashBase<T>(m, H), pr_(pr) {
    initSeeds();
  }
  UnivHash_linear(FileHandler* fin):
    HashBase<T>(fin) {
    load(fin);
  }
  ~UnivHash_linear() {
    freeSeeds();
  }
  T hash(const char* s, count_t h) {
    return 0; //not implemented
  }
  T hash(const wordID_t* id, const int len, count_t h);
  T hash(const wordID_t id, const count_t pos,
         const T prevValue, count_t h);
  void save(FileHandler* fout);
  void load(FileHandler* fin);
private:
  T** a_, **b_;
  P pr_;
  void initSeeds();
  void freeSeeds();
};

/** UnivHash_noPrimes:
 * From Dietzfelbinger 2008
 * p = input domain range = 2^l
 * m = output range = 2^k
 * # of hash function = 2^(l-1)
 */
template <typename T>
class UnivHash_noPrimes: public HashBase<T>
{
public:
  UnivHash_noPrimes(float k, float l):
    HashBase<T>(k, 100), d_(count_t((l-k))) {
    if(((int)l >> 3) == sizeof(P)) p_ = (P) pow(2,l) - 1;
    else p_ = (P) pow(2,l);
    initSeeds();
  }
  UnivHash_noPrimes(FileHandler* fin):
    HashBase<T>(fin) {
    load(fin);
  }
  ~UnivHash_noPrimes() {
    freeSeeds();
  }
  T hash(const char* s, count_t h);
  T hash(const wordID_t* id, const int len, count_t h);
  T hash(const P x, count_t h);
  void save(FileHandler* fout);
  void load(FileHandler* fin);
private:
  count_t d_;  // l-k
  P p_, *a_;   // real-valued input range, storage
  void initSeeds();
  void freeSeeds() {
    delete[] a_;
  }
};

//! @todo ask abby2
template <typename T>
class Hash_shiftAddXOR: public HashBase<T>
{
public:
  Hash_shiftAddXOR(float m, count_t H=5): HashBase<T>(m,H),
    l_(5), r_(2) {
    initSeeds();
  }
  ~Hash_shiftAddXOR() {
    freeSeeds();
  }
  T hash(const char* s, count_t h);
  T hash(const wordID_t* id, const int len, count_t h) {} // empty
private:
  T* v_;      // random seed storage
  const unsigned short l_, r_; // left-shift bits, right-shift bits
  void initSeeds();
  void freeSeeds() {
    delete[] v_;
  }
};

//! @todo ask abby2
template <typename T>
class UnivHash_tableXOR: public HashBase<T>
{
public:
  UnivHash_tableXOR(float m, count_t H=5): HashBase<T>(m, H),
    table_(NULL), tblLen_(255*MAX_STR_LEN) {
    initSeeds();
  }
  ~UnivHash_tableXOR() {
    freeSeeds();
  }
  T hash(const char* s, count_t h);
  T hash(const wordID_t* id, const int len, count_t h) {}
private:
  T** table_; // storage for random numbers
  count_t tblLen_;     // length of table
  void initSeeds();
  void freeSeeds();
};

// ShiftAddXor
template <typename T>
void Hash_shiftAddXOR<T>::initSeeds()
{
  v_ = new T[this->H_];
  for(count_t i=0; i < this->H_; i++)
    v_[i] = Utils::rand<T>() + 1;
}
template <typename T>
T Hash_shiftAddXOR<T>::hash(const char* s, count_t h)
{
  T value = v_[h];
  int pos(0);
  unsigned char c;
  while((c = *s++) && (++pos < MAX_STR_LEN)) {
    value ^= ((value << l_) + (value >> r_) + c);
  }
  return (value % this->m_);
}

// UnivHash_tableXOR
template <typename T>
void UnivHash_tableXOR<T>::initSeeds()
{
  // delete any values in table
  if(table_) freeSeeds();
  // instance of new table
  table_ = new T* [this->H_];
  // fill with random values
  for(count_t j=0; j < this->H_; j++) {
    table_[j] = new T[tblLen_];
    for(count_t i=0; i < tblLen_; i++) {
      table_[j][i] = Utils::rand<T>(this->m_-1);
    }
  }
}
template <typename T>
void UnivHash_tableXOR<T>::freeSeeds()
{
  for(count_t j = 0; j < this->H_; j++)
    delete[] table_[j];
  delete[] table_;
  table_ = NULL;
}
template <typename T>
T UnivHash_tableXOR<T>::hash(const char* s, count_t h)
{
  T value = 0;
  count_t pos = 0, idx = 0;
  unsigned char c;
  while((c = *s++) && (++pos < MAX_STR_LEN))
    value ^= table_[h][idx += c];
  UTIL_THROW_IF2(value >= this->m_, "Error");
  return value;
}

// UnivHash_noPrimes
template <typename T>
void UnivHash_noPrimes<T>::initSeeds()
{
  a_ = new P[this->H_];
  for(T i=0; i < this->H_; i++) {
    a_[i] = Utils::rand<P>();
    if(a_[i] % 2 == 0) a_[i]++;  // a must be odd
  }
}
template <typename T>
T UnivHash_noPrimes<T>::hash(const P x, count_t h)
{
  // h_a(x) = (ax mod 2^l) div 2^(l-k)
  T value = ((a_[h] * x) % p_) >> d_;
  return value % this->m_;
}
template <typename T>
T UnivHash_noPrimes<T>::hash(const wordID_t* id, const int len,
                             count_t h)
{
  T value = 0;
  int pos(0);
  while(pos < len) {
    value ^= hash((P)id[pos], h++);
    pos++;
  }
  return value % this->m_;
}
template <typename T>
T UnivHash_noPrimes<T>::hash(const char* s, count_t h)
{
  T value = 0;
  int pos(0);
  unsigned char c;
  while((c = *s++) && (++pos < MAX_STR_LEN)) {
    value ^= hash((P)c, h);
  }
  return value % this->m_;
}
template <typename T>
void UnivHash_noPrimes<T>::save(FileHandler* fout)
{
  HashBase<T>::save(fout);
  fout->write((char*)&p_, sizeof(p_));
  fout->write((char*)&d_, sizeof(d_));
  for(T i=0; i < this->H_; i++) {
    fout->write((char*)&a_[i], sizeof(a_[i]));
  }
}
template <typename T>
void UnivHash_noPrimes<T>::load(FileHandler* fin)
{
  a_ = new P[this->H_];
  // HashBase<T>::load(fin) already done in constructor
  fin->read((char*)&p_, sizeof(p_));
  fin->read((char*)&d_, sizeof(d_));
  for(T i=0; i < this->H_; i++) {
    fin->read((char*)&a_[i], sizeof(a_[i]));
  }
}

//UnivHash_linear
template <typename T>
void UnivHash_linear<T>::initSeeds()
{
  a_ = new T*[this->H_];
  b_ = new T*[this->H_];
  for(count_t i=0; i < this->H_; i++) {
    a_[i] = new T[MAX_NGRAM_ORDER];
    b_[i] = new T[MAX_NGRAM_ORDER];
    for(count_t j=0; j < MAX_NGRAM_ORDER; j++) {
      a_[i][j] = 1 + Utils::rand<T>();
      b_[i][j] = Utils::rand<T>();
    }
  }
}
template <typename T>
void UnivHash_linear<T>::freeSeeds()
{
  for(count_t i=0; i < this->H_; i++) {
    delete[] a_[i];
    delete[] b_[i];
  }
  delete[] a_;
  delete[] b_;
  a_ = b_ = NULL;
}
template <typename T>
inline T UnivHash_linear<T>::hash(const wordID_t* id, const int len,
                                  count_t h)
{
  UTIL_THROW_IF2(h >= this->H_, "Error");

  T value = 0;
  int pos(0);
  while(pos < len) {
    value += ((a_[h][pos] * id[pos]) + b_[h][pos]);// % pr_;
    ++pos;
  }
  return value % this->m_;
}
template <typename T>
inline T UnivHash_linear<T>::hash(const wordID_t id, const count_t pos,
                                  const T prevValue, count_t h)
{
  UTIL_THROW_IF2(h >= this->H_, "Error");
  T value = prevValue + ((a_[h][pos] * id) + b_[h][pos]); // % pr_;
  return value % this->m_;
}
template <typename T>
void UnivHash_linear<T>::save(FileHandler* fout)
{
  // int bytes = sizeof(a_[0][0]);
  HashBase<T>::save(fout);
  fout->write((char*)&pr_, sizeof(pr_));
  for(count_t i=0; i < this->H_; i++) {
    for(count_t j=0; j < MAX_NGRAM_ORDER; j++) {
      fout->write((char*)&a_[i][j], sizeof(a_[i][j]));
      fout->write((char*)&b_[i][j], sizeof(b_[i][j]));
      //cout << "a[" << i << "][" << j << "]=" << a_[i][j] << endl;
      //cout << "b[" << i << "][" << j << "]=" << b_[i][j] << endl;
    }
  }
}
template <typename T>
void UnivHash_linear<T>::load(FileHandler* fin)
{
  // HashBase<T>::load(fin) already done in constructor
  fin->read((char*)&pr_, sizeof(pr_));
  a_ = new T*[this->H_];
  b_ = new T*[this->H_];
  for(count_t i=0; i < this->H_; i++) {
    a_[i] = new T[MAX_NGRAM_ORDER];
    b_[i] = new T[MAX_NGRAM_ORDER];
    for(count_t j=0; j < MAX_NGRAM_ORDER; j++) {
      fin->read((char*)&a_[i][j], sizeof(a_[i][j]));
      fin->read((char*)&b_[i][j], sizeof(b_[i][j]));
      //cout << "a[" << i << "][" << j << "]=" << a_[i][j] << endl;
      //cout << "b[" << i << "][" << j << "]=" << b_[i][j] << endl;
    }
  }
}
#endif
