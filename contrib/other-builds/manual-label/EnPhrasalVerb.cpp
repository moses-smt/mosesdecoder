#include <iostream>
#include <list>
#include <limits>
#include <algorithm>
#include "EnPhrasalVerb.h"
#include "moses/Util.h"

using namespace std;

void EnPhrasalVerb(const Phrase &source, int revision, ostream &out)
{
  Ranges ranges;

  // find ranges to label
  for (int start = 0; start < source.size(); ++start) {
	size_t end = std::numeric_limits<size_t>::max();

	if (IsA(source, start, 0, 0, "ask asked asking")) {
		end = Found(source, start, 0, "out");
    }
	else if (IsA(source, start, 0, 0, "back backed backing")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "blow blown blew")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "break broke broken")) {
		end = Found(source, start, 0, "down up in");
	}
	else if (IsA(source, start, 0, 0, "bring brought bringing")) {
		end = Found(source, start, 0, "down up in");
	}
	else if (IsA(source, start, 0, 0, "call called calling")) {
		end = Found(source, start, 0, "back up off");
	}
	else if (IsA(source, start, 0, 0, "check checked checking")) {
		end = Found(source, start, 0, "out in");
	}
	else if (IsA(source, start, 0, 0, "cheer cheered cheering")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "clean cleaned cleaning")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "cross crossed crossing")) {
		end = Found(source, start, 0, "out");
	}
	else if (IsA(source, start, 0, 0, "cut cutting")) {
		end = Found(source, start, 0, "down off out");
	}
	else if (IsA(source, start, 0, 0, "do did done")) {
		end = Found(source, start, 0, "over up");
	}
	else if (IsA(source, start, 0, 0, "drop dropped dropping")) {
		end = Found(source, start, 0, "off");
	}
	else if (IsA(source, start, 0, 0, "figure figured figuring")) {
		end = Found(source, start, 0, "out");
	}
	else if (IsA(source, start, 0, 0, "fill filled filling")) {
		end = Found(source, start, 0, "in out up");
	}
	else if (IsA(source, start, 0, 0, "find found finding")) {
		end = Found(source, start, 0, "out");
	}
	else if (IsA(source, start, 0, 0, "get got getting gotten")) {
		end = Found(source, start, 0, "across over back");
	}
	else if (IsA(source, start, 0, 0, "give given gave giving")) {
		end = Found(source, start, 0, "away back out up");
	}
	else if (IsA(source, start, 0, 0, "hand handed handing")) {
		end = Found(source, start, 0, "down in over");
	}
	else if (IsA(source, start, 0, 0, "hold held holding")) {
		end = Found(source, start, 0, "back up");
	}
	else if (IsA(source, start, 0, 0, "keep kept keeping")) {
		end = Found(source, start, 0, "from up");
	}
	else if (IsA(source, start, 0, 0, "let letting")) {
		end = Found(source, start, 0, "down in");
	}
	else if (IsA(source, start, 0, 0, "look looked looking")) {
		end = Found(source, start, 0, "over up");
	}
	else if (IsA(source, start, 0, 0, "make made making")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "mix mixed mixing")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "pass passed passing")) {
		end = Found(source, start, 0, "out up");
	}
	else if (IsA(source, start, 0, 0, "pay payed paying")) {
		end = Found(source, start, 0, "back");
	}
	else if (IsA(source, start, 0, 0, "pick picked picking")) {
		end = Found(source, start, 0, "out");
	}
	else if (IsA(source, start, 0, 0, "point pointed pointing")) {
		end = Found(source, start, 0, "out");
	}
	else if (IsA(source, start, 0, 0, "put putting")) {
		end = Found(source, start, 0, "down off out together on");
	}
	else if (IsA(source, start, 0, 0, "send sending")) {
		end = Found(source, start, 0, "back");
	}
	else if (IsA(source, start, 0, 0, "set setting")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "sort sorted sorting")) {
		end = Found(source, start, 0, "out");
	}
	else if (IsA(source, start, 0, 0, "switch switched switching")) {
		end = Found(source, start, 0, "off on");
	}
	else if (IsA(source, start, 0, 0, "take took taking")) {
		end = Found(source, start, 0, "apart back off out");
	}
	else if (IsA(source, start, 0, 0, "tear torn tearing")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "think thought thinking")) {
		end = Found(source, start, 0, "over");
	}
	else if (IsA(source, start, 0, 0, "thrown threw thrown throwing")) {
		end = Found(source, start, 0, "away");
	}
	else if (IsA(source, start, 0, 0, "turn turned turning")) {
		end = Found(source, start, 0, "down off on");
	}
	else if (IsA(source, start, 0, 0, "try tried trying")) {
		end = Found(source, start, 0, "on out");
	}
	else if (IsA(source, start, 0, 0, "use used using")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "warm warmed warming")) {
		end = Found(source, start, 0, "up");
	}
	else if (IsA(source, start, 0, 0, "work worked working")) {
		end = Found(source, start, 0, "out");
	}

	// found range to label
	if (end != std::numeric_limits<size_t>::max() &&
			end > start + 1) {
		bool add = true;
		if (revision == 1 && Exist(source,
									start + 1,
									end - 1,
									1,
									"VB VBD VBG VBN VBP VBZ")) {
			// there's a verb in between
			add = false;
		}

		if (add) {
			Range range(start + 1, end - 1, "reorder-label");
			ranges.push_back(range);
		}
	}
  }

  OutputWithLabels(source, ranges, out);
}

bool Exist(const Phrase &source, int start, int end, int factor, const std::string &str)
{
  vector<string> soughts = Moses::Tokenize(str, " ");
  for (size_t i = start; i <= end; ++i) {
	const Word &word = source[i];
    bool found = Found(word, factor, soughts);
    if (found) {
    	return true;
    }
  }

  return false;
}

size_t Found(const Phrase &source, int pos, int factor, const std::string &str)
{
  const size_t MAX_RANGE = 10;

  vector<string> soughts = Moses::Tokenize(str, " ");
  vector<string> puncts = Moses::Tokenize(". : , ;", " ");


  size_t maxEnd = std::min(source.size(), (size_t) pos + MAX_RANGE);
  for (size_t i = pos + 1; i < maxEnd; ++i) {
	const Word &word = source[i];
	bool found;

	found = Found(word, factor, puncts);
	if (found) {
		return std::numeric_limits<size_t>::max();
	}

	found = Found(word, factor, soughts);
	if (found) {
		return i;
	}
  }

  return std::numeric_limits<size_t>::max();
}


bool Found(const Word &word, int factor, const vector<string> &soughts)
{
  const string &element = word[factor];
  for (size_t i = 0; i < soughts.size(); ++i) {
	const string &sought = soughts[i];
	bool found = (element == sought);
	if (found) {
	  return true;
	}
  }
  return false;
}


