#include "ScorerFactory.h"

#include <stdexcept>
#include "Scorer.h"
#include "BleuScorer.h"
#include "PerScorer.h"
#include "TerScorer.h"
#include "CderScorer.h"
#include "MergeScorer.h"

using namespace std;

vector<string> ScorerFactory::getTypes() {
  vector<string> types;
  types.push_back(string("BLEU"));
  types.push_back(string("PER"));
  types.push_back(string("TER"));
  types.push_back(string("CDER"));
  types.push_back(string("MERGE"));
  return types;
}

Scorer* ScorerFactory::getScorer(const string& type, const string& config) {
  if (type == "BLEU") {
    return (BleuScorer*) new BleuScorer(config);
  } else if (type == "PER") {
    return (PerScorer*) new PerScorer(config);
  } else if (type == "TER") {
    return (TerScorer*) new TerScorer(config);
  } else if (type == "CDER") {
    return (CderScorer*) new CderScorer(config);
  } else if (type == "MERGE") {
    return (MergeScorer*) new MergeScorer(config);
  } else {
    throw runtime_error("Unknown scorer type: " + type);
  }
}
