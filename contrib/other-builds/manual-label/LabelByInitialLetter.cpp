#include "LabelByInitialLetter.h"
#include "Main.h"

using namespace std;

void LabelByInitialLetter(const Phrase &source, std::ostream &out)
{
  Ranges ranges;

  for (int start = 0; start < source.size(); ++start) {
	  const string &startWord = source[start][0];
	  string startChar = startWord.substr(0,1);

	  for (int end = start + 1; end < source.size(); ++end) {
		  const string &endWord = source[end][0];
		  string endChar = endWord.substr(0,1);

		  if (startChar == endChar) {
				Range range(start, end, startChar + "-label");
				ranges.push_back(range);
		  }
	  }
  }

  OutputWithLabels(source, ranges, out);

}


