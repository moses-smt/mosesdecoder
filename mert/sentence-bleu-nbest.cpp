#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "BleuScorer.h"
#include "Reference.h"
#include "moses/Util.h"
#include "util/exception.hh"

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

  // initialize reference streams
  std::vector<boost::shared_ptr<std::ifstream> > refStreams;
  for (std::vector<std::string>::const_iterator refFile=refFiles.begin(); refFile!=refFiles.end(); ++refFile) {
    TRACE_ERR("Loading reference from " << *refFile << std::endl);
    boost::shared_ptr<std::ifstream> ifs(new std::ifstream(refFile->c_str()));
    UTIL_THROW_IF2(!ifs, "Cannot open " << *refFile);
    refStreams.push_back(ifs);
  }

  // load sentences, preparing statistics, score
  std::string nbestLine;
  int sid = -1;
  Reference ref;
  while ( getline(std::cin, nbestLine) ) {
    std::vector<std::string> items;
    Moses::TokenizeMultiCharSeparator(items, nbestLine, " ||| ");
    int sidCurrent = Moses::Scan<int>(items[0]);

    if (sidCurrent != sid) {
      ref.clear();
      if (!scorer.GetNextReferenceFromStreams(refStreams, ref)) {
        UTIL_THROW2("Missing references");
      }
      sid = sidCurrent;
    }
    ScoreStats scoreStats;
    scorer.CalcBleuStats(ref, items[1], scoreStats);
    std::vector<float> stats(scoreStats.getArray(), scoreStats.getArray() + scoreStats.size());
    std::cout << smoothedSentenceBleu(stats) << std::endl;
  }

  return 0;
}

