#include "FF.h"
#include <sstream>

using namespace std;

std::string FF::OutputFactors(const std::vector<int> &factors) const
{  
  if (factors.size() == 0) 
    return "";
 
  stringstream ret; 
  ret << factors[0];
  for (size_t i = 1; i < factors.size(); ++i) {
    ret << "," << factors[i];
  }
  return ret.str();
}
