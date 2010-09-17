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
#include "PermutationScorer.h"
#include "InterpolatedScorer.h"
#include "PerScorer.h"

using namespace std;

class ScorerFactory {

    public:
        vector<string> getTypes() {
            vector<string> types;
            types.push_back(string("BLEU1"));
            types.push_back(string("BLEU"));
            types.push_back(string("PER"));
            types.push_back(string("HAMMING"));
            types.push_back(string("KENDALL"));
            return types;
        }

        Scorer* getScorer(const string& type, const string& config = "") {
            size_t scorerTypes = type.find(",");
            if(scorerTypes == string::npos) {
                if (type == "BLEU1") {
									string conf;
									if (config.length() > 0) {
								    conf = config +  ",ngramlen:1";
									} else {
								    conf = config +  "ngramlen:1";
									}
                  return (BleuScorer*) new BleuScorer(conf);
                } else if (type == "BLEU") {
                  return (BleuScorer*) new BleuScorer(config);
                } else if (type == "PER") {
                  return (PerScorer*) new PerScorer(config);
                } else if ((type == "HAMMING") || (type == "KENDALL")) {
                  return (PermutationScorer*) new PermutationScorer(type, config);
                } else {
                  throw runtime_error("Unknown scorer type: " + type);
                }
            } else {
                  return (InterpolatedScorer*) new InterpolatedScorer(type, config);
            }
       }
};

#endif //__SCORER_FACTORY_H
