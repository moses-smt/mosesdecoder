#include "DynSuffixArray.h"
#include <iostream>
namespace Moses {
DynSuffixArray::DynSuffixArray() {
  SA_ = new vuint_t();
  ISA_ = new vuint_t();
  F_ = new vuint_t();
  L_ = new vuint_t();
  std::cerr << "DYNAMIC SUFFIX ARRAY CLASS INSTANTIATED" << std::endl;
}
DynSuffixArray::~DynSuffixArray() {
  delete SA_;
  delete ISA_;
  delete F_;
  delete L_;
}
DynSuffixArray::DynSuffixArray(vuint_t* crp) {
  // make native int array and pass to SA builder
  corpus_ = crp; 
  int size = corpus_->size();
  int* tmpArr = new int[size];
  for(int i=0 ; i < size; ++i) tmpArr[i] = i; 
  qsort(tmpArr, 0, size-1);
  SA_ = new vuint_t(tmpArr, tmpArr + size);
  //std::cerr << "printing SA " << std::endl;
  //for(int i=0; i < size; ++i) std::cerr << SA_->at(i) << std::endl;
  delete[] tmpArr;
  std::cerr << "DYNAMIC SUFFIX ARRAY CLASS INSTANTIATED WITH SIZE " << size << std::endl;
  buildAuxArrays();
  //printAuxArrays();
}
void DynSuffixArray::buildAuxArrays() {
  int size = SA_->size();
  ISA_ = new vuint_t(size);
  F_ = new vuint_t(size);
  L_ = new vuint_t(size);
  for(int i=0; i < size; ++i) {
    ISA_->at(SA_->at(i)) = i;
    //(*ISA_)[(*SA_)[i]] = i;
    (*F_)[i] = (*corpus_)[SA_->at(i)];
    (*L_)[i] = (*corpus_)[(SA_->at(i) == 0 ? size-1 : SA_->at(i)-1)]; 
  }
}
int DynSuffixArray::rank(unsigned word, unsigned idx) {
/* use Gerlach's code to make rank faster */
  // the number of word in L[0..i]
  int r(0);
  for(unsigned i=0; i < idx; ++i)
    if(L_->at(i) == word) ++r;
  return r;
}
/* count function should be implemented
 * with binary search over suffix array!! */
int DynSuffixArray::F_firstIdx(unsigned word) {
  // return index of first row where word is found in F_
  int low = std::lower_bound(F_->begin(), F_->end(), word) - F_->begin();
  if(F_->at(low) == word) return low;
  else return -1;
}
/* uses rank() and c() to obtain the LF function */
int DynSuffixArray::LF(unsigned L_idx) {
  int fIdx(-1);
  unsigned word = L_->at(L_idx);
  if((fIdx = F_firstIdx(word)) != -1) 
    return fIdx + rank(word, L_idx);
}
void DynSuffixArray::insertFactor(vuint_t* newSent, unsigned newIndex) {
  // for sentences
  //stages 1, 2, 4 stay same from 1char case
  //(use last word of new text in step 2 and save Ltmp until last insert?)
  //stage 3...all words of new sentence are inserted backwards
  // stage 2: k=ISA[newIndex], tmp= L[k], L[k]  = newChar
  assert(newIndex <= SA_->size());
  int k(-1), kprime(-1);
  k = (newIndex < SA_->size() ? ISA_->at(newIndex) : ISA_->at(0)); // k is now index of the cycle that starts at newindex
  int true_pos = LF(k); // track cycle shift (newIndex - 1)
  int Ltmp = L_->at(k);
  L_->at(k) = (*newSent)[newSent->size()-1];  // cycle k now ends with correct word
  for(int j = newSent->size()-1; j > -1; --j) {
    kprime = LF(k);  // find cycle that starts with (newindex - 1)
    //kprime += ((L_[k] == Ltmp) && (k > isa[k]) ? 1 : 0); // yada yada
    // only terminal char can be 0 so add new vocab at end
    kprime = (kprime > 0 ? kprime : SA_->size());  
    true_pos += (kprime <= true_pos ? 1 : 0); // track changes
    // insert everything
    F_->insert(F_->begin() + kprime, (*newSent)[j]);
    int theLWord = (j == 0 ? Ltmp : (*newSent)[j-1]);
    L_->insert(L_->begin() + kprime, theLWord);
    piterate(SA_, itr) 
      if(*itr >= newIndex) ++(*itr);
    SA_->insert(SA_->begin() + kprime, newIndex);
    piterate(ISA_, itr)
      if(*itr >= kprime) ++(*itr);
    ISA_->insert(ISA_->begin() + newIndex, kprime);
    k = kprime;
  }
  // Begin stage 4
  reorder(true_pos, LF(kprime)); // actual position vs computed position of cycle (newIndex-1)
}
void DynSuffixArray::reorder(unsigned j, unsigned jprime) {
  printf("j=%d\tj'=%d\n", j, jprime);
  while(j != jprime) {
    printf("j=%d\tj'=%d\n", j, jprime);
    int tmp, isaIdx(-1);
    int new_j = LF(j);
    // for SA, L, and F, the element at pos j is moved to j'
    tmp = L_->at(j); // L
    L_->at(j) = L_->at(jprime);
    L_->at(jprime) = tmp;
    tmp = SA_->at(j);  // SA
    SA_->at(j) = SA_->at(jprime);
    SA_->at(jprime) = tmp;
    // all ISA values between (j...j'] decremented
    for(int i = 0; i < ISA_->size(); ++i) {
      if((ISA_->at(i) == j) && (isaIdx == -1)) 
        isaIdx = i; // store index of ISA[i] = j
      if((ISA_->at(i) > j) && (ISA_->at(i) <= jprime)) --(*ISA_)[i];
    }
    // replace j with j' in ISA
    //isa[isaIdx] = jprime;
    ISA_->at(isaIdx) = jprime;
    j = new_j;
    jprime = LF(jprime);
  }
}
void DynSuffixArray::deleteFactor(unsigned index, unsigned num2del) {
  int ltmp = L_->at(ISA_->at(index));
  int true_pos = LF(ISA_->at(index)); // track cycle shift (newIndex - 1)
  for(int q = 0; q < num2del; ++q) {
    int row = ISA_->at(index); // gives the position of index in SA and F_
    std::cerr << "row = " << row << std::endl;
    std::cerr << "SA[r]/index = " << SA_->at(row) << "/" << index << std::endl;
    true_pos -= (row <= true_pos ? 1 : 0); // track changes
    L_->erase(L_->begin() + row);     
    F_->erase(F_->begin() + row);     
    ISA_->erase(ISA_->begin() + index);  // order is important     
    piterate(ISA_, itr)
      if(*itr > row) --(*itr);
    SA_->erase(SA_->begin() + row);
    piterate(SA_, itr)
      if(*itr > index) --(*itr);
  }
  L_->at(ISA_->at(index))= ltmp;
  reorder(LF(ISA_->at(index)), true_pos);
  printAuxArrays();
}
void DynSuffixArray::substituteFactor(vuint_t* newSents, unsigned newIndex) {
  std::cerr << "NEEDS TO IMPELEMNT SUBSITITUTE FACTOR\n";
  return;
}
bool DynSuffixArray::getCorpusIndex(const vuint_t* phrase, vuint_t* indices) {
  pair<vuint_t::iterator,vuint_t::iterator> bounds;
  indices->clear();
  int phrasesize = phrase->size();
  // find lower and upper bounds on phrase[0]
  bounds = std::equal_range(F_->begin(), F_->end(), phrase->at(0));
  // bounds holds first and (last + 1) index of phrase[0] in SA_
  int lwrBnd = int(bounds.first - F_->begin());
  int uprBnd = int(bounds.second - F_->begin());
  if(uprBnd - lwrBnd == 0) return false;  // not found
  if(phrasesize == 1) {
    for(int i=lwrBnd; i < uprBnd; ++i) {
      indices->push_back(SA_->at(i));
    }
    return (indices->size() > 0);
  }
  //find longer phrases if they exist
  for(int i = lwrBnd; i < uprBnd; ++i) {
    int crpIdx = SA_->at(i);
    if((crpIdx + phrasesize) >= corpus_->size()) continue; // past end of corpus
    for(int pos = 1; pos < phrasesize; ++pos) { // for all following words
      if(corpus_->at(crpIdx + pos) != phrase->at(pos)) {  // if word doesn't match
        if(indices->size() > 0) i = uprBnd;  // past the phrases since SA is ordered
        break; 
      }
      else if(pos == phrasesize-1) { // found phrase 
        indices->push_back(crpIdx + pos);  // store rigthmost index of phrase 
      }
    }  
  }
  //cerr << "Total count of phrase = " << indices->size() << endl;
  return (indices->size() > 0);
}
void DynSuffixArray::save(FILE* fout) {
  fWriteVector(fout, *SA_);
}
void DynSuffixArray::load(FILE* fin) {
  fReadVector(fin, *SA_);
}
int DynSuffixArray::compare(int pos1, int pos2, int max) {
  for (int i=0; i < max; ++i) {
    if((pos1 + i < corpus_->size()) && (pos2 + i >= corpus_->size()))
      return 1;
    if((pos2 + i < corpus_->size()) && (pos1 + i >= corpus_->size()))
      return -1;
  
    int diff = corpus_->at(pos1+i) - corpus_->at(pos2+i);
    if(diff != 0) return diff;
  }
  return 0;
}
void DynSuffixArray::qsort(int* array, int begin, int end) {
  if(end > begin) 
  {
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
        if (compare(array[i], pivot, 20) <= 0) {
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
    qsort(array, begin, index - 1);
    qsort(array, index + 1,  end);
  }
}



} // end namespace
