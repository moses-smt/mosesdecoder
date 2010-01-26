#ifndef MOSES_DYNSFXARY_H
#define MOSES_DYNSFXARY_H

#include <vector>
namespace Moses {
using std::vector;

class DynSuffixArray {
public:
  DynSuffixArray();
  ~DynSuffixArray();
private: 
  vector<unsigned>* SA_;
  vector<unsigned>* ISA_;
  vector<unsigned>* F_;
  vector<unsigned>* L_;
  void reorder(unsigned, unsigned);
};

} //end namespace
#endif
