#pragma once

#include <string>
#include <set>
#include <vector>

namespace Moses
{

typedef std::vector<int> MinPhrase;

struct MinPhraseSorter {
    bool operator()(const MinPhrase& p1, const MinPhrase& p2) {
        if(p1[1] != -1 && p2[1] != -1)
            return p1[1] < p2[1];
        else if(p1[0] != -1 && p2[0] != -1)
            return p1[0] < p2[0];
        else if(p1[0] == -1)
            return true;
        else if(p2[0] == -1)
            return false;
        return false;
    }
};

typedef std::set<MinPhrase, MinPhraseSorter> MinPhrases;

bool overlap(const MinPhrase& p1, const MinPhrase& p2);

MinPhrase combine(const MinPhrase& p1, const MinPhrase& p2);

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>&,
                      const std::vector<std::string>&,
                      const std::vector<size_t>&); 
}