#ifndef moses_DynSuffixArray_h
#define moses_DynSuffixArray_h

#include <vector>
#include <set>
#include <algorithm>
#include <utility>
#include "Util.h"
#include "File.h"
#include "DynSAInclude/types.h"

namespace Moses
{

typedef std::vector<unsigned> vuint_t;

/** @todo ask Abbey Levenberg
 */
class DynSuffixArray
{

public:
  DynSuffixArray();
  DynSuffixArray(vuint_t*);
  ~DynSuffixArray();
  bool GetCorpusIndex(const vuint_t*, vuint_t*);
  void Load(FILE*);
  void Save(FILE*);
  void Insert(vuint_t*, unsigned);
  void Delete(unsigned, unsigned);
  void Substitute(vuint_t*, unsigned);

private:
  vuint_t* m_SA;
  vuint_t* m_ISA;
  vuint_t* m_F;
  vuint_t* m_L;
  vuint_t* m_corpus;
  void BuildAuxArrays();
  void Qsort(int* array, int begin, int end);
  int Compare(int, int, int);
  void Reorder(unsigned, unsigned);
  int LastFirstFunc(unsigned);
  int Rank(unsigned, unsigned);
  int F_firstIdx(unsigned);
  void PrintAuxArrays() {
    std::cerr << "SA\tISA\tF\tL\n";
    for(size_t i=0; i < m_SA->size(); ++i)
      std::cerr << m_SA->at(i) << "\t" << m_ISA->at(i) << "\t" << m_F->at(i) << "\t" << m_L->at(i) << std::endl;
  }
};

} //end namespace

#endif
