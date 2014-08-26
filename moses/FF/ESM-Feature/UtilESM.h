#pragma once

#include <string>
#include <set>
#include <vector>

namespace Moses
{

typedef std::vector<int> MinPhrase;

struct MinPhraseSorter {
    bool operator()(const MinPhrase& p1, const MinPhrase& p2) {
        // both have target positions, order by target
        if(p1[1] != -1 && p2[1] != -1)
            return p1[1] < p2[1];
        
        // both have source positions, order by source
        if(p1[0] != -1 && p2[0] != -1)
            return p1[0] < p2[0];
        
        // only first has target position
        if(p1[1] != -1 && p2[1] == -1)
            return p1[0] < p2[1];
        
        // only second has target position
        if(p1[1] == -1 && p2[1] != -1)
            return p1[1] < p2[0];
        
        return false;
    }
};

typedef std::set<MinPhrase, MinPhraseSorter> MinPhrases;

bool overlap(const MinPhrase& p1, const MinPhrase& p2);

MinPhrase combine(const MinPhrase& p1, const MinPhrase& p2);

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>&,
                      const std::vector<std::string>&); 

std::vector<std::string> calculateEdits(
                      const std::vector<std::string>&,
                      const std::vector<std::string>&,
                      const std::vector<size_t>&); 
}