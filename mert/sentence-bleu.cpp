#include <iostream>
#include <vector>
#include <string>

#include "BleuScorer.h"

using namespace std;
using namespace MosesTuning;

int main(int argc, char **argv)
{
  if (argc == 1) {
    cerr << "Usage: ./sentence-bleu ref1 [ref2 ...] < candidate > bleu-scores" << endl;
    return 1;
  }
  vector<string> refFiles(argv + 1, argv + argc);

  // TODO all of these are empty for now
  string config;
  string factors;
  string filter;

  BleuScorer scorer(config);
  scorer.setFactors(factors);
  scorer.setFilter(filter);
  scorer.setReferenceFiles(refFiles);

  vector<ScoreStats> entries;

  // Loading sentences and preparing statistics
  ScoreStats scoreentry;
  string line;
  while (getline(cin, line)) {
    scorer.prepareStats(entries.size(), line, scoreentry);
    entries.push_back(scoreentry);
  }

  vector<ScoreStats>::const_iterator sentIt;
  for (sentIt = entries.begin(); sentIt != entries.end(); sentIt++) {
    vector<float> stats(sentIt->getArray(), sentIt->getArray() + sentIt->size());
    cout << smoothedSentenceBleu(stats) << "\n";
  }
  return 0;
}
