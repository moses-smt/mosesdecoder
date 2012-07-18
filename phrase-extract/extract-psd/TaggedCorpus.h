#ifndef __TAGGED_CORPUS_HPP_
#define __TAGGED_CORPUS_HPP_

#include <vector>
#include <string>

using namespace std;

vector< vector<string> > parseTaggedString(const string& input, const string& delimiter, size_t factorCount);

#endif
