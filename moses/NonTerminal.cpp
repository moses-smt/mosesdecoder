
#include "NonTerminal.h"

using namespace std;

namespace Moses
{
std::ostream& operator<<(std::ostream &out, const NonTerminalSet &obj)
{
  NonTerminalSet::const_iterator iter;
  for (iter = obj.begin(); iter != obj.end(); ++iter) {
    const Word &word = *iter;
    out << word << " ";
  }


  return out;
}


}
