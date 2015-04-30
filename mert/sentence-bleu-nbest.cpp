#include <iostream>
#include <vector>
#include <string>

#include "BleuScorer.h"
#include "moses/Util.h"

using namespace MosesTuning;

int main(int argc, char **argv)
{
  if (argc == 1) {
    std::cerr << "Usage: ./sentence-bleu-nbest ref1 [ref2 ...] < plain-nbest > bleu-scores" << std::endl;
    return 1;
  }

  std::vector<std::string> refFiles(argv + 1, argv + argc);

  // TODO all of these are empty for now
  std::string config;
  std::string factors;
  std::string filter;

  BleuScorer scorer(config);
  scorer.setFactors(factors);
  scorer.setFilter(filter);
  scorer.setReferenceFiles(refFiles); // TODO: we don't need to load the whole reference corpus into memory (this can take gigabytes of RAM if done with millions of sentences)

  // Loading sentences and preparing statistics
  std::string nbestLine;
  while ( getline(std::cin, nbestLine) ) 
  {
    std::vector<std::string> items;
    Moses::TokenizeMultiCharSeparator(items, nbestLine, " ||| ");
    size_t sid = Moses::Scan<size_t>(items[0]);

    ScoreStats scoreStats;
    scorer.prepareStats(sid, items[1], scoreStats);
    std::vector<float> stats(scoreStats.getArray(), scoreStats.getArray() + scoreStats.size());
    std::cout << smoothedSentenceBleu(stats) << std::endl;
  }

  return 0;
}
