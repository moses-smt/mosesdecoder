#include "FF.h"

std::ostream& operator<<(std::ostream &out, const FF &model)
{
  model.Output(out);
  return out;
}

