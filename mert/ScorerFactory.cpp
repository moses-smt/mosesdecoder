#include "ScorerFactory.h"

#include <stdexcept>
#include "Scorer.h"
#include "BleuScorer.h"
#include "BleuDocScorer.h"
#include "PerScorer.h"
#include "TerScorer.h"
#include "CderScorer.h"
#include "InterpolatedScorer.h"
#include "SemposScorer.h"
#include "PermutationScorer.h"
#include "Reference.h"

using namespace std;

namespace MosesTuning
{


vector<string> ScorerFactory::getTypes()
{
  vector<string> types;
  types.push_back(string("BLEU"));
  types.push_back(string("BLEUDOC"));
  types.push_back(string("PER"));
  types.push_back(string("TER"));
  types.push_back(string("CDER"));
  types.push_back(string("WER"));
  types.push_back(string("MERGE"));
  types.push_back(string("SEMPOS"));
  types.push_back(string("LRSCORE"));
  return types;
}

Scorer* ScorerFactory::getScorer(const string& type, const string& config)
{
  if (type == "BLEU") {
    return new BleuScorer(config);
  } else if (type == "BLEUDOC") {
    return new BleuDocScorer(config);
  } else if (type == "PER") {
    return new PerScorer(config);
  } else if (type == "TER") {
    return new TerScorer(config);
  } else if (type == "CDER") {
    return new CderScorer(config, true);
  } else if (type == "WER") {
    // CderScorer can compute both CDER and WER metric
    return new CderScorer(config, false);
  } else if (type == "SEMPOS") {
    return new SemposScorer(config);
  } else if ((type == "HAMMING") || (type == "KENDALL")) {
    return (PermutationScorer*) new PermutationScorer(type, config);
  } else {
    if (type.find(',') != string::npos) {
      return new InterpolatedScorer(type, config);
    } else {
      throw runtime_error("Unknown scorer type: " + type);
    }
  }
}

}

