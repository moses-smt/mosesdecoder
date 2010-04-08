#ifndef moses_DynSuffixArray_h
#define moses_DynSuffixArray_h

#include <vector>
#include <set>
#include <algorithm>
#include <utility>
#include "Util.h"
#include "File.h"
#include "DynSAInclude/types.h"

namespace Moses {

using std::vector;
using std::pair;

typedef std::vector<unsigned> vuint_t;

class DynSuffixArray {

public:
  DynSuffixArray();
  DynSuffixArray(vuint_t*);
  ~DynSuffixArray();
  bool getCorpusIndex(const vuint_t*, vuint_t*);
  void load(FILE*);
  void save(FILE*);

private: 
  vuint_t* m_SA;
  vuint_t* m_ISA;
  vuint_t* m_F;
  vuint_t* m_L;
  vuint_t* m_corpus;
  void buildAuxArrays();
  void qsort(int* array, int begin, int end);
  int compare(int, int, int);
  void reorder(unsigned, unsigned);
  void insertFactor(vuint_t*, unsigned);  
  void deleteFactor(unsigned, unsigned);
  void substituteFactor(vuint_t*, unsigned);
  int LF(unsigned);
  int rank(unsigned, unsigned);
  int F_firstIdx(unsigned);
  void printAuxArrays() {
    std::cerr << "SA\tISA\tF\tL\n";
    for(size_t i=0; i < m_SA->size(); ++i)
      std::cerr << m_SA->at(i) << "\t" << m_ISA->at(i) << "\t" << m_F->at(i) << "\t" << m_L->at(i) << std::endl;
  }
};

} //end namespace

#endif
