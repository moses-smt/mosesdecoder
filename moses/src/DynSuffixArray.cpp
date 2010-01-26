#include "DynSuffixArray.h"
#include <iostream>
namespace Moses {
DynSuffixArray::DynSuffixArray() {
  SA_ = new vector<unsigned>();
  ISA_ = new vector<unsigned>();
  F_ = new vector<unsigned>();
  L_ = new vector<unsigned>();
  std::cerr << "DYNAMIC SUFFIX ARRAY CLASS INSTANTIATED" << std::endl;
}
DynSuffixArray::~DynSuffixArray() {
  delete SA_;
  delete ISA_;
  delete F_;
  delete L_;
}
} // end namespace
