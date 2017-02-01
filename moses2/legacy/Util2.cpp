#include "Util2.h"
#include "util/exception.hh"

namespace Moses2
{

class BoolValueException: public util::Exception
{
};

template<>
bool Scan<bool>(const std::string &input)
{
  std::string lc = ToLower(input);
  if (lc == "yes" || lc == "y" || lc == "true" || lc == "1") return true;
  if (lc == "no" || lc == "n" || lc == "false" || lc == "0") return false;
  UTIL_THROW(BoolValueException,
             "Could not interpret " << input << " as a boolean.  After lowercasing, valid values are yes, y, true, 1, no, n, false, and 0.");
}

const std::string ToLower(const std::string& str)
{
  std::string lc(str);
  std::transform(lc.begin(), lc.end(), lc.begin(), (int (*)(int))std::tolower);
  return
    lc  ;
}

}

