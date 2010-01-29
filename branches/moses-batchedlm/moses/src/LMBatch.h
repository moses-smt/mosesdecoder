#pragma once

#include <set>

namespace Moses {

typedef std::vector<std::string *> Tail;

class Ngram {
	public:
		std::string * head;
		Tail * tail;
		float prob;
};

class NgramCmp {
	public:
		bool operator()(Ngram * k1, Ngram * k2);
};

typedef std::set<Ngram *, NgramCmp> NgramSet;

}
