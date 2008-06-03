#ifndef __SCORER_FACTORY_H
#define __SCORER_FACTORY_H

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Types.h"
#include "Scorer.h"
#include "BleuScorer.h"
#include "PerScorer.h"

using namespace std;

class ScorerFactory {

    public:
        vector<string> getTypes() {
            vector<string> types;
            types.push_back(string("BLEU"));
            types.push_back(string("PER"));
            return types;
        }

        Scorer* getScorer(const string& type) {
						if (type == "BLEU") {
							return (BleuScorer*) new BleuScorer();
            } else if (type == "PER") {
							return (PerScorer*) new PerScorer();
            } else {
                throw runtime_error("Unknown scorer type: " + type);
            }
       }
};

#endif //__SCORER_FACTORY_H
