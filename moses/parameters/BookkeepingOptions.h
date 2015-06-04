// -*- mode: c++; cc-style: gnu -*-
#include "moses/Parameter.h"
// #include <string>

namespace Moses
{

struct BookkeepingOptions {
  bool need_alignment_info;
  bool init(Parameter const& param);
};



}
