#pragma once

#include <string>
#include <set>
#include <vector>

namespace Moses
{

typedef std::vector<size_t> MinPhrase;
typedef std::set<MinPhrase> MinPhrases;

bool overlap(const MinPhrase& p1, const MinPhrase& p2);

MinPhrase combine(const MinPhrase& p1, const MinPhrase& p2);

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>&,
                      const std::vector<std::string>&,
                      const std::vector<size_t>&); 
}