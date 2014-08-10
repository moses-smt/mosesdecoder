#include <list>
#include "DeEn.h"
#include "Main.h"
#include "moses/Util.h"

using namespace std;

extern bool g_debug;

bool Contains(const Phrase &source, int start, int end, int factor, const string &str)
{
  for (int pos = start; pos <= end; ++pos) {
    bool found = IsA(source, pos, 0, factor, str);
    if (found) {
      return true;
    }
  }
  return false;
}

void LabelDeEn(const Phrase &source, ostream &out)
{
  Ranges ranges;

  // find ranges to label
  for (int start = 0; start < source.size(); ++start) {
    for (int end = start; end < source.size(); ++end) {
     if (IsA(source, start, -1, 1, "VAFIN")
          && IsA(source, end, +1, 1, "VVINF VVPP")
          && !Contains(source, start, end, 1, "VAFIN VVINF VVPP VVFIN")) {
       Range range(start, end, "reorder-label");
       ranges.push_back(range);
      }
      else if ((start == 0 || IsA(source, start, -1, 1, "$,"))
          && IsA(source, end, +1, 0, "zu")
          && IsA(source, end, +2, 1, "VVINF")
          && !Contains(source, start, end, 1, "$,")) {
        Range range(start, end, "reorder-label");
        ranges.push_back(range);
      }
    }
  }

  OutputWithLabels(source, ranges, out);
}

