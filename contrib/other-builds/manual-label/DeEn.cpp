#include <list>
#include "DeEn.h"
#include "moses/Util.h"

using namespace std;

extern bool g_debug;

bool IsA(const Phrase &source, int pos, int offset, int factor, const string &str)
{
  pos += offset;
  if (pos >= source.size() || pos < 0) {
    return false;
  }

  const string &word = source[pos][factor];
  vector<string> soughts = Moses::Tokenize(str, " ");
  for (int i = 0; i < soughts.size(); ++i) {
    string &sought = soughts[i];
    bool found = (word == sought);
    if (found) {
      return true;
    }
  }
  return false;
}

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
  typedef pair<int,int> Range;
  typedef list<Range> Ranges;
  Ranges ranges;

  // find ranges to label
  for (int start = 0; start < source.size(); ++start) {
    for (int end = start; end < source.size(); ++end) {
     if (IsA(source, start, -1, 1, "VAFIN")
          && IsA(source, end, +1, 1, "VVINF VVPP")
          && !Contains(source, start, end, 1, "VAFIN VVINF VVPP VVFIN")) {
       Range range(start, end);
       ranges.push_back(range);
      }
      else if ((start == 0 || IsA(source, start, -1, 1, "$,"))
          && IsA(source, end, +1, 0, "zu")
          && IsA(source, end, +2, 1, "VVINF")
          && !Contains(source, start, end, 1, "$,")) {
        Range range(start, end);
        ranges.push_back(range);
      }
    }
  }

  // output sentence, with labels
  for (int pos = 0; pos < source.size(); ++pos) {
    // output beginning of label
    for (Ranges::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) {
      const Range &range = *iter;
      if (range.first == pos) {
        out << "<tree label=\"REORDER-LABEL\"> ";
      }
    }

    const Word &word = source[pos];
    out << word[0] << " ";

    for (Ranges::const_iterator iter = ranges.begin(); iter != ranges.end(); ++iter) {
      const Range &range = *iter;
      if (range.second == pos) {
        out << "</tree> ";
      }
    }
  }
  out << endl;

}
