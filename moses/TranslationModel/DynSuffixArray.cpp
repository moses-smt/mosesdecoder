#include "DynSuffixArray.h"
#include <iostream>
#include <boost/foreach.hpp>

using namespace std;

namespace Moses
{

DynSuffixArray::DynSuffixArray()
{
  m_SA = new vuint_t();
  m_ISA = new vuint_t();
  m_F = new vuint_t();
  m_L = new vuint_t();
  std::cerr << "DYNAMIC SUFFIX ARRAY CLASS INSTANTIATED" << std::endl;
}

DynSuffixArray::~DynSuffixArray()
{
  delete m_SA;
  delete m_ISA;
  delete m_F;
  delete m_L;
}

DynSuffixArray::DynSuffixArray(vuint_t* crp)
{
  // make native int array and pass to SA builder
  m_corpus = crp;
  int size = m_corpus->size();
  int* tmpArr = new int[size];
  for(int i=0 ; i < size; ++i) tmpArr[i] = i;

  Qsort(tmpArr, 0, size-1);

  m_SA = new vuint_t(tmpArr, tmpArr + size);
  //std::cerr << "printing SA " << std::endl;
  //for(int i=0; i < size; ++i) std::cerr << m_SA->at(i) << std::endl;
  delete[] tmpArr;
  std::cerr << "DYNAMIC SUFFIX ARRAY CLASS INSTANTIATED WITH SIZE " << size << std::endl;
  BuildAuxArrays();
  //printAuxArrays();
}

void DynSuffixArray::BuildAuxArrays()
{
  int size = m_SA->size();
  m_ISA = new vuint_t(size);
  m_F = new vuint_t(size);
  m_L = new vuint_t(size);

  for(int i=0; i < size; ++i) {
    m_ISA->at(m_SA->at(i)) = i;
    //(*m_ISA)[(*m_SA)[i]] = i;
    (*m_F)[i] = (*m_corpus)[m_SA->at(i)];
    (*m_L)[i] = (*m_corpus)[(m_SA->at(i) == 0 ? size-1 : m_SA->at(i)-1)];
  }
}

int DynSuffixArray::Rank(unsigned word, unsigned idx)
{
  /* use Gerlach's code to make rank faster */
  // the number of words in L[0..i] (minus 1 which is why 'i < idx', not '<=')
  int r(0);
  for(unsigned i=0; i < idx; ++i)
    if(m_L->at(i) == word) ++r;
  return r;
}

/* count function should be implemented
 * with binary search over suffix array!! */
int DynSuffixArray::F_firstIdx(unsigned word)
{
  // return index of first row where word is found in m_F
  /*for(int i=0; i < m_F->size(); ++i) {
    if(m_F->at(i) == word) {
      return i;
    }
  }
  return -1;*/
  //NOTE: lower_bound  is faster than linear search above but may cause issues
  //      if ordering of vocab is not consecutive (ie..after deletions)
  int low = std::lower_bound(m_F->begin(), m_F->end(), word) - m_F->begin();
  //cerr << "in F_firstIdx with word = " << word << " and low = " << low <<  " and F->size() =" << m_F->size() << endl;
  if((size_t)low >= m_F->size())
    return -1;
  else
    return low;
}

/* uses rank() and c() to obtain the LastFirstFunc function */
int DynSuffixArray::LastFirstFunc(unsigned L_idx)
{
  int fIdx(-1);
  //cerr << "in LastFirstFcn() with L_idx = " << L_idx << endl;
  unsigned word = m_L->at(L_idx);
  if((fIdx = F_firstIdx(word)) != -1) {
    //cerr << "fidx + Rank(" << word << "," << L_idx << ") = " << fIdx << "+" << Rank(word, L_idx) << endl;
    fIdx += Rank(word, L_idx);
  }
  return fIdx;
}

void DynSuffixArray::Insert(vuint_t* newSent, unsigned newIndex)
{
  // for sentences
  //stages 1, 2, 4 stay same from 1char case
  //(use last word of new text in step 2 and save Ltmp until last insert?)
  //stage 3...all words of new sentence are inserted backwards
  // stage 2: k=ISA[newIndex], tmp= L[k], L[k]  = newChar
  //PrintAuxArrays();
  UTIL_THROW_IF2(newIndex > m_SA->size(), "Error");
  int k(-1), kprime(-1);
  k = (newIndex < m_SA->size() ? m_ISA->at(newIndex) : m_ISA->at(0)); // k is now index of the cycle that starts at newindex
  int true_pos = LastFirstFunc(k); // track cycle shift (newIndex - 1)
  int Ltmp = m_L->at(k);
  m_L->at(k) = newSent->at(newSent->size()-1);  // cycle k now ends with correct word
  for(int j = newSent->size()-1; j > -1; --j) {
    kprime = LastFirstFunc(k);  // find cycle that starts with (newindex - 1)
    //kprime += ((m_L[k] == Ltmp) && (k > isa[k]) ? 1 : 0); // yada yada
    // only terminal char can be 0 so add new vocab at end
    kprime = (kprime > 0 ? kprime : m_SA->size());
    true_pos += (kprime <= true_pos ? 1 : 0); // track changes
    // insert everything
    m_F->insert(m_F->begin() + kprime, newSent->at(j));
    int theLWord = (j == 0 ? Ltmp : newSent->at(j-1));

    m_L->insert(m_L->begin() + kprime, theLWord);
    for (vuint_t::iterator itr = m_SA->begin(); itr != m_SA->end(); ++itr) {
      if(*itr >= newIndex) ++(*itr);
    }
    m_SA->insert(m_SA->begin() + kprime, newIndex);
    for (vuint_t::iterator itr = m_ISA->begin(); itr != m_ISA->end(); ++itr) {
      if((int)*itr >= kprime) ++(*itr);
    }

    m_ISA->insert(m_ISA->begin() + newIndex, kprime);
    k = kprime;
    //PrintAuxArrays();
  }
  // Begin stage 4
  Reorder(true_pos, LastFirstFunc(kprime)); // actual position vs computed position of cycle (newIndex-1)
}

void DynSuffixArray::Reorder(unsigned j, unsigned jprime)
{
  set<pair<unsigned, unsigned> > seen;
  while(j != jprime) {
    // this 'seenit' check added for data with many loops. will remove after double
    // checking.
    bool seenit = seen.insert(std::make_pair(j, jprime)).second;
    if(seenit) {
      for(size_t i=1; i < m_SA->size(); ++i) {
        if(m_corpus->at(m_SA->at(i)) < m_corpus->at(m_SA->at(i-1))) {
          cerr << "PROBLEM WITH SUFFIX ARRAY REORDERING. EXITING...\n";
          exit(1);
        }
      }
      return;
    }
    //cerr << "j=" << j << "\tj'=" << jprime << endl;
    int isaIdx(-1);
    int new_j = LastFirstFunc(j);
    UTIL_THROW_IF2(j > jprime, "Error");
    // for SA and L, the element at pos j is moved to pos j'
    m_L->insert(m_L->begin() + jprime + 1, m_L->at(j));
    m_L->erase(m_L->begin() + j);
    m_SA->insert(m_SA->begin() + jprime + 1, m_SA->at(j));
    m_SA->erase(m_SA->begin() + j);
    // all ISA values between (j...j'] decremented
    for(size_t i = 0; i < m_ISA->size(); ++i) {
      if((m_ISA->at(i) == j) && (isaIdx == -1))
        isaIdx = i; // store index of ISA[i] = j
      if((m_ISA->at(i) > j) && (m_ISA->at(i) <= jprime)) --(*m_ISA)[i];
    }
    // replace j with j' in ISA
    //isa[isaIdx] = jprime;
    m_ISA->at(isaIdx) = jprime;
    j = new_j;
    jprime = LastFirstFunc(jprime);
  }
  //cerr << "j=" << j << "\tj'=" << jprime << endl;
}

void DynSuffixArray::Delete(unsigned index, unsigned num2del)
{
  int ltmp = m_L->at(m_ISA->at(index));
  int true_pos = LastFirstFunc(m_ISA->at(index)); // track cycle shift (newIndex - 1)
  for(size_t q = 0; q < num2del; ++q) {
    int row = m_ISA->at(index); // gives the position of index in SA and m_F
    //std::cerr << "row = " << row << std::endl;
    //std::cerr << "SA[r]/index = " << m_SA->at(row) << "/" << index << std::endl;
    true_pos -= (row <= true_pos ? 1 : 0); // track changes
    m_L->erase(m_L->begin() + row);
    m_F->erase(m_F->begin() + row);

    m_ISA->erase(m_ISA->begin() + index);  // order is important
    for (vuint_t::iterator itr = m_ISA->begin(); itr != m_ISA->end(); ++itr) {
      if((int)*itr > row) --(*itr);
    }

    m_SA->erase(m_SA->begin() + row);
    for (vuint_t::iterator itr = m_SA->begin(); itr != m_SA->end(); ++itr) {
      if(*itr > index) --(*itr);
    }
  }
  m_L->at(m_ISA->at(index))= ltmp;
  Reorder(LastFirstFunc(m_ISA->at(index)), true_pos);
  //PrintAuxArrays();
}

void DynSuffixArray::Substitute(vuint_t* /* newSents */, unsigned /* newIndex */)
{
  std::cerr << "NEEDS TO IMPLEMENT SUBSITITUTE FACTOR\n";
  return;
}

ComparePosition::
ComparePosition(vuint_t const& crp, vuint_t const& sfa)
  : m_crp(crp), m_sfa(sfa) { }

bool
ComparePosition::
operator()(unsigned const& i, vector<wordID_t> const& phrase) const
{
  unsigned const* x = &m_crp.at(i);
  unsigned const* e = &m_crp.back();
  size_t k = 0;
  for (; k < phrase.size() && x < e; ++k, ++x)
    if (*x != phrase[k]) return *x < phrase[k];
  return (x == e && k < phrase.size());
}

bool
ComparePosition::
operator()(vector<wordID_t> const& phrase, unsigned const& i) const
{
  unsigned const* x = &m_crp.at(i);
  unsigned const* e = &m_crp.back();
  size_t k = 0;
  for (; k < phrase.size() && x < e; ++k, ++x)
    if (*x != phrase[k]) return phrase[k] < *x;
  return false; // (k == phrase.size() && x < e);
}

bool DynSuffixArray::GetCorpusIndex(const vuint_t* phrase, vuint_t* indices)
{
  // DOES THIS EVEN WORK WHEN A DynSuffixArray has been saved and reloaded????
  pair<vuint_t::iterator,vuint_t::iterator> bounds;
  indices->clear();
  size_t phrasesize = phrase->size();
  // find lower and upper bounds on phrase[0]
  bounds = std::equal_range(m_F->begin(), m_F->end(), phrase->at(0));
  // bounds holds first and (last + 1) index of phrase[0] in m_SA
  size_t lwrBnd = size_t(bounds.first - m_F->begin());
  size_t uprBnd = size_t(bounds.second - m_F->begin());
  //cerr << "phrasesize = " << phrasesize << "\tuprBnd = " << uprBnd << "\tlwrBnd = " << lwrBnd;
  //cerr << "\tcorpus size =  " << m_corpus->size() << endl;
  if(uprBnd - lwrBnd == 0) return false;  // not found
  if(phrasesize == 1) {
    for(size_t i=lwrBnd; i < uprBnd; ++i) {
      indices->push_back(m_SA->at(i));
    }
    return (indices->size() > 0);
  }
  //find longer phrases if they exist
  for(size_t i = lwrBnd; i < uprBnd; ++i) {
    size_t crpIdx = m_SA->at(i);
    if((crpIdx + phrasesize) > m_corpus->size()) continue; // past end of corpus
    for(size_t pos = 1; pos < phrasesize; ++pos) { // for all following words
      if(m_corpus->at(crpIdx + pos) != phrase->at(pos)) {  // if word doesn't match
        if(indices->size() > 0) i = uprBnd;  // past the phrases since SA is ordered
        break;
      } else if(pos == phrasesize-1) { // found phrase
        indices->push_back(crpIdx + pos);  // store rigthmost index of phrase
      }
    }
  }
  //cerr << "Total count of phrase = " << indices->size() << endl;
  return (indices->size() > 0);
}

size_t
DynSuffixArray::
GetCount(vuint_t const& phrase) const
{
  ComparePosition cmp(*m_corpus, *m_SA);
  vuint_t::const_iterator lb = lower_bound(m_SA->begin(), m_SA->end(), phrase, cmp);
  vuint_t::const_iterator ub = upper_bound(m_SA->begin(), m_SA->end(), phrase, cmp);
  return ub-lb;
}

void DynSuffixArray::Save(FILE* fout)
{
  fWriteVector(fout, *m_SA);
}

void DynSuffixArray::Load(FILE* fin)
{
  fReadVector(fin, *m_SA);
}

int DynSuffixArray::Compare(int pos1, int pos2, int max)
{
  for (size_t i = 0; i < (unsigned)max; ++i) {
    if((pos1 + i < m_corpus->size()) && (pos2 + i >= m_corpus->size()))
      return 1;
    if((pos2 + i < m_corpus->size()) && (pos1 + i >= m_corpus->size()))
      return -1;

    int diff = m_corpus->at(pos1+i) - m_corpus->at(pos2+i);
    if(diff != 0) return diff;
  }
  return 0;
}

void DynSuffixArray::Qsort(int* array, int begin, int end)
{
  if(end > begin) {
    int index;
    {
      index = begin + (rand() % (end - begin + 1));
      int pivot = array[index];
      {
        int tmp = array[index];
        array[index] = array[end];
        array[end] = tmp;
      }
      for(int i=index=begin; i < end; ++i) {
        if (Compare(array[i], pivot, 20) <= 0) {
          {
            int tmp = array[index];
            array[index] = array[i];
            array[i] = tmp;
            index++;
          }
        }
      }
      {
        int tmp = array[index];
        array[index] = array[end];
        array[end] = tmp;
      }
    }
    Qsort(array, begin, index - 1);
    Qsort(array, index + 1,  end);
  }
}



} // end namespace
