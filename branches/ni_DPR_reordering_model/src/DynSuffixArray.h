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
  vuint_t* SA_;
  vuint_t* ISA_;
  vuint_t* F_;
  vuint_t* L_;
  vuint_t* corpus_;
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
    std::cerr << "SA\tISA\tF_\tL_\n";
    for(int i=0; i < SA_->size(); ++i)
      std::cerr << SA_->at(i) << "\t" << ISA_->at(i) << "\t" << F_->at(i) << "\t" << L_->at(i) << std::endl;
  }
};

} //end namespace

#endif
