
#include "Util.h"

const std::string Trim(const std::string& str, const std::string dropChars)
{
  std::string res = str;
  res.erase(str.find_last_not_of(dropChars)+1);
  return res.erase(0, res.find_first_not_of(dropChars));
}
