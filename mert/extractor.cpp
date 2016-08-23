/**
 * Extract features and score statistics from nvest file, optionally merging with
 * those from the previous iteration.
 * Developed during the 2nd MT marathon.
 **/

#include <iostream>
#include <string>
#include <vector>

#include <getopt.h>
#include <boost/scoped_ptr.hpp>

#include "Data.h"
#include "Scorer.h"
#include "ScorerFactory.h"
#include "Timer.h"
#include "Util.h"

using namespace std;
using namespace MosesTuning;

namespace
{

void usage()
{
  cerr << "usage: extractor [options])" << endl;
  cerr << "[--sctype|-s] the scorer type (default BLEU)" << endl;
  cerr << "[--scconfig|-c] configuration string passed to scorer" << endl;
  cerr << "\tThis is of the form NAME1:VAL1,NAME2:VAL2 etc " << endl;
  cerr << "[--reference|-r] comma separated list of reference files" << endl;
  cerr << "[--binary|-b] use binary output format (default to text )" << endl;
  cerr << "[--nbest|-n] the nbest file" << endl;
  cerr << "[--scfile|-S] the scorer data output file" << endl;
  cerr << "[--ffile|-F] the feature data output file" << endl;
  cerr << "[--prev-ffile|-E] comma separated list of previous feature data" << endl;
  cerr << "[--prev-scfile|-R] comma separated list of previous scorer data" << endl;
  cerr << "[--factors|-f] list of factors passed to the scorer (e.g. 0|2)" << endl;
  cerr << "[--filter|-l] filter command used to preprocess the sentences" << endl;
  cerr << "[--allow-duplicates|-d] omit the duplicate removal step" << endl;
  cerr << "[-v] verbose level" << endl;
  cerr << "[--help|-h] print this message and exit" << endl;
  exit(1);
}

static struct option long_options[] = {
  {"sctype", required_argument, 0, 's'},
  {"scconfig", required_argument,0, 'c'},
  {"factors", required_argument,0, 'f'},
  {"filter", required_argument,0, 'l'},
  {"reference", required_argument, 0, 'r'},
  {"binary", no_argument, 0, 'b'},
  {"nbest", required_argument, 0, 'n'},
  {"scfile", required_argument, 0, 'S'},
  {"ffile", required_argument, 0, 'F'},
  {"prev-scfile", required_argument, 0, 'R'},
  {"prev-ffile", required_argument, 0, 'E'},
  {"verbose", required_argument, 0, 'v'},
  {"help", no_argument, 0, 'h'},
  {"allow-duplicates", no_argument, 0, 'd'},
  {0, 0, 0, 0}
};

// Command line options used in extractor.
struct ProgramOption {
  string scorerType;
  string scorerConfig;
  string scorerFactors;
  string scorerFilter;
  string referenceFile;
  string nbestFile;
  string scoreDataFile;
  string featureDataFile;
  string prevScoreDataFile;
  string prevFeatureDataFile;
  bool binmode;
  bool allowDuplicates;
  int verbosity;

  ProgramOption()
    : scorerType("BLEU"),
      scorerConfig(""),
      scorerFactors(""),
      scorerFilter(""),
      referenceFile(""),
      nbestFile(""),
      scoreDataFile("statscore.data"),
      featureDataFile("features.data"),
      prevScoreDataFile(""),
      prevFeatureDataFile(""),
      binmode(false),
      allowDuplicates(false),
      verbosity(0) { }
};

void ParseCommandOptions(int argc, char** argv, ProgramOption* opt)
{
  int c;
  int option_index;

  while ((c = getopt_long(argc, argv, "s:r:f:l:n:S:F:R:E:v:hbd", long_options, &option_index)) != -1) {
    switch (c) {
    case 's':
      opt->scorerType = string(optarg);
      break;
    case 'c':
      opt->scorerConfig = string(optarg);
      break;
    case 'f':
      opt->scorerFactors = string(optarg);
      break;
    case 'l':
      opt->scorerFilter = string(optarg);
      break;
    case 'r':
      opt->referenceFile = string(optarg);
      break;
    case 'b':
      opt->binmode = true;
      break;
    case 'n':
      opt->nbestFile = string(optarg);
      break;
    case 'S':
      opt->scoreDataFile = string(optarg);
      break;
    case 'F':
      opt->featureDataFile = string(optarg);
      break;
    case 'E':
      opt->prevFeatureDataFile = string(optarg);
      break;
    case 'R':
      opt->prevScoreDataFile = string(optarg);
      break;
    case 'v':
      opt->verbosity = atoi(optarg);
      break;
    case 'd':
      opt->allowDuplicates = true;
      break;
    default:
      usage();
    }
  }
}

} // anonymous namespace

int main(int argc, char** argv)
{
  ResetUserTime();

  ProgramOption option;
  ParseCommandOptions(argc, argv, &option);

  try {
    // check whether score statistics file is specified
    if (option.scoreDataFile.length() == 0) {
      throw runtime_error("Error: output score statistics file is not specified");
    }

    // check wheter feature file is specified
    if (option.featureDataFile.length() == 0) {
      throw runtime_error("Error: output feature file is not specified");
    }

    // check whether reference file is specified when nbest is specified
    if ((option.nbestFile.length() > 0 && option.referenceFile.length() == 0)) {
      throw runtime_error("Error: reference file is not specified; you can not score the nbest");
    }

    vector<string> nbestFiles;
    if (option.nbestFile.length() > 0) {
      Tokenize(option.nbestFile.c_str(), ',', &nbestFiles);
    }

    vector<string> referenceFiles;
    if (option.referenceFile.length() > 0) {
      Tokenize(option.referenceFile.c_str(), ',', &referenceFiles);
    }

    vector<string> prevScoreDataFiles;
    if (option.prevScoreDataFile.length() > 0) {
      Tokenize(option.prevScoreDataFile.c_str(), ',', &prevScoreDataFiles);
    }

    vector<string> prevFeatureDataFiles;
    if (option.prevFeatureDataFile.length() > 0) {
      Tokenize(option.prevFeatureDataFile.c_str(), ',', &prevFeatureDataFiles);
    }

    if (prevScoreDataFiles.size() != prevFeatureDataFiles.size()) {
      throw runtime_error("Error: there is a different number of previous score and feature files");
    }

    if (option.binmode) {
      cerr << "Binary write mode is selected" << endl;
    } else {
      cerr << "Binary write mode is NOT selected" << endl;
    }

    TRACE_ERR("Scorer type: " << option.scorerType << endl);

    boost::scoped_ptr<Scorer> scorer(
      ScorerFactory::getScorer(option.scorerType, option.scorerConfig));

    // set Factors and Filter used to preprocess the sentences
    scorer->setFactors(option.scorerFactors);
    scorer->setFilter(option.scorerFilter);

    // load references
    if (referenceFiles.size() > 0)
      scorer->setReferenceFiles(referenceFiles);

//    PrintUserTime("References loaded");

    Data data(scorer.get());

    // load old data
    for (size_t i = 0; i < prevScoreDataFiles.size(); i++) {
      data.load(prevFeatureDataFiles.at(i), prevScoreDataFiles.at(i));
    }

//    PrintUserTime("Previous data loaded");

    // computing score statistics of each nbest file
    for (size_t i = 0; i < nbestFiles.size(); i++) {
      data.loadNBest(nbestFiles.at(i));
    }

//    PrintUserTime("Nbest entries loaded and scored");

    //ADDED_BY_TS
    if (!option.allowDuplicates) {
      data.removeDuplicates();
    }
    //END_ADDED

    data.save(option.featureDataFile, option.scoreDataFile, option.binmode);
    PrintUserTime("Stopping...");

    return EXIT_SUCCESS;
  } catch (const exception& e) {
    cerr << "Exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }
}
