#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "BleuScorer.h"
#include "Reference.h"
#include "moses/Util.h"
#include "util/exception.hh"

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

  // initialize reference streams
  vector<boost::shared_ptr<ifstream> > refStreams;
  for (vector<string>::const_iterator refFile=refFiles.begin(); refFile!=refFiles.end(); ++refFile) {
    TRACE_ERR("Loading reference from " << *refFile << endl);
    boost::shared_ptr<ifstream> ifs(new ifstream(refFile->c_str()));
    UTIL_THROW_IF2(!ifs, "Cannot open " << *refFile);
    refStreams.push_back(ifs);
  }

  // load sentences, preparing statistics, score
  string hypothesisLine;
  size_t sid = 0;
  while (getline(std::cin, hypothesisLine)) {
    Reference ref;
    if (!scorer.GetNextReferenceFromStreams(refStreams, ref)) {
      UTIL_THROW2("Missing references");
    }
    ScoreStats scoreStats;
    scorer.CalcBleuStats(ref, hypothesisLine, scoreStats);
    vector<float> stats(scoreStats.getArray(), scoreStats.getArray() + scoreStats.size());
    std::cout << smoothedSentenceBleu(stats) << std::endl;
    ++sid;
  }

  return 0;
}

