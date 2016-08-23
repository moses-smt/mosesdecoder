/**
 * \description This is the main for the new version of the mert algorithm developed during the 2nd MT marathon
*/

#include <limits>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>

#include <getopt.h>
#include <boost/scoped_ptr.hpp>

#include "Data.h"
#include "Point.h"
#include "Scorer.h"
#include "ScorerFactory.h"
#include "ScoreData.h"
#include "FeatureData.h"
#include "Optimizer.h"
#include "OptimizerFactory.h"
#include "Types.h"
#include "Timer.h"
#include "Util.h"
#include "util/random.hh"

#include "moses/ThreadPool.h"

using namespace std;
using namespace MosesTuning;

namespace
{

const char kDefaultOptimizer[] = "powell";
const char kDefaultScorer[] = "BLEU";
const char kDefaultScorerFile[] = "statscore.data";
const char kDefaultFeatureFile[] = "features.data";
const char kDefaultInitFile[] = "init.opt";
const char kDefaultPositiveString[] = "";
const char kDefaultSparseWeightsFile[] = "";

// Used when saving optimized weights.
const char kOutputFile[] = "weights.txt";

/**
 * Runs an optimisation, or a random restart.
 */
class OptimizationTask : public Moses::Task
{
public:
  OptimizationTask(Optimizer* optimizer, const Point& point)
    : m_optimizer(optimizer), m_point(point) {}

  ~OptimizationTask() {}

  virtual void Run() {
    m_score = m_optimizer->Run(m_point);
  }

  virtual bool DeleteAfterExecution() {
    return false;
  }

  void resetOptimizer() {
    if (m_optimizer) {
      delete m_optimizer;
      m_optimizer = NULL;
    }
  }

  statscore_t getScore() const {
    return m_score;
  }

  const Point& getPoint() const  {
    return m_point;
  }

private:
  // Do not allow the user to instanciate without arguments.
  OptimizationTask() {}

  Optimizer* m_optimizer;
  Point m_point;
  statscore_t m_score;
};

bool WriteFinalWeights(const char* filename, const Point& point)
{
  ofstream ofs(filename);
  if (!ofs) {
    cerr << "Cannot open " << filename << endl;
    return false;
  }

  ofs << point << endl;

  return true;
}


void usage(int ret)
{
  cerr<<"usage: mert -d <dimensions> (mandatory )"<<endl;
  cerr<<"[-n] retry ntimes (default 1)"<<endl;
  cerr<<"[-m] number of random directions in powell (default 0)"<<endl;
  cerr<<"[-o] the indexes to optimize(default all)"<<endl;
  cerr<<"[-t] the optimizer(default powell)"<<endl;
  cerr<<"[-r] the random seed (defaults to system clock)"<<endl;
  cerr<<"[--sctype|-s] the scorer type (default BLEU)"<<endl;
  cerr<<"[--scconfig|-c] configuration string passed to scorer"<<endl;
  cerr<<"[--scfile|-S] comma separated list of scorer data files (default score.data)"<<endl;
  cerr<<"[--ffile|-F] comma separated list of feature data files (default feature.data)"<<endl;
  cerr<<"[--ifile|-i] the starting point data file (default init.opt)"<<endl;
  cerr<<"[--sparse-weights|-p] required for merging sparse features"<<endl;
#ifdef WITH_THREADS
  cerr<<"[--threads|-T] use multiple threads (default 1)"<<endl;
#endif
  cerr<<"[--shard-count] Split data into shards, optimize for each shard and average"<<endl;
  cerr<<"[--shard-size] Shard size as proportion of data. If 0, use non-overlapping shards"<<endl;
  cerr<<"[-v] verbose level"<<endl;
  cerr<<"[--help|-h] print this message and exit"<<endl;
  exit(ret);
}

static struct option long_options[] = {
  {"pdim", 1, 0, 'd'},
  {"ntry",1,0,'n'},
  {"nrandom",1,0,'m'},
  {"rseed",required_argument,0,'r'},
  {"optimize",1,0,'o'},
  {"type",1,0,'t'},
  {"sctype",1,0,'s'},
  {"scconfig",required_argument,0,'c'},
  {"scfile",1,0,'S'},
  {"ffile",1,0,'F'},
  {"ifile",1,0,'i'},
  {"sparse-weights",required_argument,0,'p'},
#ifdef WITH_THREADS
  {"threads", required_argument,0,'T'},
#endif
  {"shard-count", required_argument, 0, 'a'},
  {"shard-size", required_argument, 0, 'b'},
  {"verbose",1,0,'v'},
  {"help",no_argument,0,'h'},
  {0, 0, 0, 0}
};

struct ProgramOption {
  string to_optimize_str;
  int pdim;
  int ntry;
  int nrandom;
  int seed;
  bool has_seed;
  string optimize_type;
  string scorer_type;
  string scorer_config;
  string scorer_file;
  string feature_file;
  string init_file;
  string positive_string;
  string sparse_weights_file;
  size_t num_threads;
  float shard_size;
  size_t shard_count;

  ProgramOption()
    : to_optimize_str(""),
      pdim(-1),
      ntry(1),
      nrandom(0),
      seed(0),
      has_seed(false),
      optimize_type(kDefaultOptimizer),
      scorer_type(kDefaultScorer),
      scorer_config(""),
      scorer_file(kDefaultScorerFile),
      feature_file(kDefaultFeatureFile),
      init_file(kDefaultInitFile),
      positive_string(kDefaultPositiveString),
      sparse_weights_file(kDefaultSparseWeightsFile),
      num_threads(1),
      shard_size(0),
      shard_count(0) { }
};

void ParseCommandOptions(int argc, char** argv, ProgramOption* opt)
{
  int c;
  int option_index;

  while ((c = getopt_long(argc, argv, "o:r:d:n:m:t:s:S:F:v:p:P:", long_options, &option_index)) != -1) {
    switch (c) {
    case 'o':
      opt->to_optimize_str = string(optarg);
      break;
    case 'd':
      opt->pdim = strtol(optarg, NULL, 10);
      break;
    case 'n':
      opt->ntry = strtol(optarg, NULL, 10);
      break;
    case 'm':
      opt->nrandom = strtol(optarg, NULL, 10);
      break;
    case 'r':
      opt->seed = strtol(optarg, NULL, 10);
      opt->has_seed = true;
      break;
    case 't':
      opt->optimize_type = string(optarg);
      break;
    case's':
      opt->scorer_type = string(optarg);
      break;
    case 'c':
      opt->scorer_config = string(optarg);
      break;
    case 'S':
      opt->scorer_file = string(optarg);
      break;
    case 'F':
      opt->feature_file = string(optarg);
      break;
    case 'i':
      opt->init_file = string(optarg);
      break;
    case 'p':
      opt->sparse_weights_file=string(optarg);
      break;
    case 'v':
      setverboselevel(strtol(optarg, NULL, 10));
      break;
#ifdef WITH_THREADS
    case 'T':
      opt->num_threads = strtol(optarg, NULL, 10);
      if (opt->num_threads < 1) opt->num_threads = 1;
      break;
#endif
    case 'a':
      opt->shard_count = strtof(optarg, NULL);
      break;
    case 'b':
      opt->shard_size = strtof(optarg, NULL);
      break;
    case 'h':
      usage(0);
      break;
    case 'P':
      opt->positive_string = string(optarg);
      break;
    default:
      usage(1);
    }
  }
}

} // anonymous namespace

int main(int argc, char **argv)
{
  ResetUserTime();

  ProgramOption option;
  ParseCommandOptions(argc, argv, &option);

  vector<unsigned> to_optimize;
  vector<vector<parameter_t> > start_list;
  vector<parameter_t> min;
  vector<parameter_t> max;
  vector<bool> positive;
  // NOTE: those mins and max are the bound for the starting points of the algorithm, not strict bound on the result!

  if (option.pdim < 0)
    usage(1);

  cerr << "shard_size = " << option.shard_size << " shard_count = " << option.shard_count << endl;
  if (option.shard_size && !option.shard_count) {
    cerr << "Error: shard-size provided without shard-count" << endl;
    exit(1);
  }
  if (option.shard_size > 1 || option.shard_size < 0) {
    cerr << "Error: shard-size should be between 0 and 1" << endl;
    exit(1);
  }

  if (option.has_seed) {
    cerr << "Seeding random numbers with " << option.seed << endl;
    util::rand_init(option.seed);
  } else {
    cerr << "Seeding random numbers with system clock " << endl;
    util::rand_init();
  }

  if (option.sparse_weights_file.size()) ++option.pdim;

  // read in starting points
  string onefile;
  while (!option.init_file.empty()) {
    getNextPound(option.init_file, onefile, ",");
    vector<parameter_t> start;
    ifstream opt(onefile.c_str());
    if (opt.fail()) {
      cerr << "could not open initfile: " << option.init_file << endl;
      exit(3);
    }
    start.resize(option.pdim);//to do:read from file
    int j;
    for (j = 0; j < option.pdim && !opt.fail(); j++) {
      opt >> start[j];
    }
    if (j < option.pdim) {
      cerr << option.init_file << ":Too few starting weights." << endl;
      exit(3);
    }
    start_list.push_back(start);
    // for the first time, also read in the min/max values for scores
    if (start_list.size() == 1) {
      min.resize(option.pdim);
      for (j = 0; j < option.pdim && !opt.fail(); j++) {
        opt >> min[j];
      }
      if (j < option.pdim) {
        cerr << option.init_file << ":Too few minimum weights." << endl;
        cerr << "error could not initialize start point with " << option.init_file << endl;
        cerr << "j: " << j << ", pdim: " << option.pdim << endl;
        exit(3);
      }
      max.resize(option.pdim);
      for (j = 0; j < option.pdim && !opt.fail(); j++) {
        opt >> max[j];
      }
      if (j < option.pdim) {
        cerr << option.init_file << ":Too few maximum weights." << endl;
        exit(3);
      }
    }
    opt.close();
  }

  vector<string> ScoreDataFiles;
  if (option.scorer_file.length() > 0) {
    Tokenize(option.scorer_file.c_str(), ',', &ScoreDataFiles);
  }

  vector<string> FeatureDataFiles;
  if (option.feature_file.length() > 0) {
    Tokenize(option.feature_file.c_str(), ',', &FeatureDataFiles);
  }

  if (ScoreDataFiles.size() != FeatureDataFiles.size()) {
    throw runtime_error("Error: there is a different number of previous score and feature files");
  }

  // it make sense to know what parameter set were used to generate the nbest
  boost::scoped_ptr<Scorer> scorer(
    ScorerFactory::getScorer(option.scorer_type, option.scorer_config));

  //load data
  Data data(scorer.get(), option.sparse_weights_file);

  for (size_t i = 0; i < ScoreDataFiles.size(); i++) {
    cerr<<"Loading Data from: "<< ScoreDataFiles.at(i) << " and " << FeatureDataFiles.at(i) << endl;
    data.load(FeatureDataFiles.at(i), ScoreDataFiles.at(i));
  }

  scorer->setScoreData(data.getScoreData().get());

  data.removeDuplicates();

  PrintUserTime("Data loaded");

  // starting point score over latest n-best, accumulative n-best
  //vector<unsigned> bests;
  //compute bests with sparse features needs to be implemented
  //currently sparse weights are not even loaded
  //statscore_t score = TheScorer->score(bests);

  if (option.to_optimize_str.length() > 0) {
    cerr << "Weights to optimize: " << option.to_optimize_str << endl;

    // Parse the string to get weights to optimize, and set them as active.
    vector<string> features;
    Tokenize(option.to_optimize_str.c_str(), ',', &features);

    for (vector<string>::const_iterator it = features.begin();
         it != features.end(); ++it) {
      const int feature_index = data.getFeatureIndex(*it);

      // Note: previous implementaion checked whether
      // feature_index is less than option.pdim.
      // However, it does not make sense when we optimize 'discrete' features,
      // given by '-o' option like -o "d_0,lm_0,tm_2,tm_3,tm_4,w_0".
      if (feature_index < 0) {
        cerr << "Error: invalid feature index = " << feature_index << endl;
        exit(1);
      }
      cerr << "FeatNameIndex: " << feature_index << " to insert" << endl;
      to_optimize.push_back(feature_index);
    }
  } else {
    //set all weights as active
    to_optimize.resize(option.pdim);//We'll optimize on everything
    for (int i = 0; i < option.pdim; i++) {
      to_optimize[i] = 1;
    }
  }

  positive.resize(option.pdim);
  for (int i = 0; i < option.pdim; i++)
    positive[i] = false;
  if (option.positive_string.length() > 0) {
    // Parse string to get weights that need to be positive
    std::string substring;
    int index;
    while (!option.positive_string.empty()) {
      getNextPound(option.positive_string, substring, ",");
      index = data.getFeatureIndex(substring);
      //index = strtol(substring.c_str(), NULL, 10);
      if (index >= 0 && index < option.pdim) {
        positive[index] = true;
      } else {
        cerr << "Index " << index
             << " is out of bounds in positivity list. Allowed indexes are [0,"
             << (option.pdim-1) << "]." << endl;
      }
    }
  }

#ifdef WITH_THREADS
  cerr << "Creating a pool of " << option.num_threads << " threads" << endl;
  Moses::ThreadPool pool(option.num_threads);
#endif

  Point::setpdim(option.pdim);
  Point::setdim(to_optimize.size());
  Point::set_optindices(to_optimize);

  //starting points consist of specified points and random restarts
  vector<Point> startingPoints;

  for (size_t i = 0; i < start_list.size(); ++i) {
    startingPoints.push_back(Point(start_list[i], min, max));
  }

  for (int i = 0; i < option.ntry; ++i) {
    startingPoints.push_back(Point(start_list[0], min, max));
    startingPoints.back().Randomize();
  }

  vector<vector<boost::shared_ptr<OptimizationTask> > > allTasks(1);

  //optional sharding
  vector<Data> shards;
  if (option.shard_count) {
    data.createShards(option.shard_count, option.shard_size, option.scorer_config, shards);
    allTasks.resize(option.shard_count);
  }

  // launch tasks
  for (size_t i = 0; i < allTasks.size(); ++i) {
    Data& data_ref = data;
    if (option.shard_count)
      data_ref = shards[i]; //use the sharded data if it exists

    vector<boost::shared_ptr<OptimizationTask> >& tasks = allTasks[i];
    Optimizer *optimizer = OptimizerFactory::BuildOptimizer(option.pdim, to_optimize, positive, start_list[0], option.optimize_type, option.nrandom);
    optimizer->SetScorer(data_ref.getScorer());
    optimizer->SetFeatureData(data_ref.getFeatureData());
    // A task for each start point
    for (size_t j = 0; j < startingPoints.size(); ++j) {
      boost::shared_ptr<OptimizationTask>
      task(new OptimizationTask(optimizer, startingPoints[j]));
      tasks.push_back(task);
#ifdef WITH_THREADS
      pool.Submit(task);
#else
      task->Run();
#endif
    }
  }

  // wait for all threads to finish
#ifdef WITH_THREADS
  pool.Stop(true);
#endif

  statscore_t total = 0;
  Point totalP;

  // collect results
  for (size_t i = 0; i < allTasks.size(); ++i) {
    statscore_t best = 0, mean = 0, var = 0;
    Point bestP;
    for (size_t j = 0; j < allTasks[i].size(); ++j) {
      statscore_t score = allTasks[i][j]->getScore();
      mean += score;
      var += score * score;
      if (score > best) {
        bestP = allTasks[i][j]->getPoint();
        best = score;
      }
    }

    mean /= static_cast<float>(option.ntry);
    var /= static_cast<float>(option.ntry);
    var = sqrt(abs(var - mean * mean));

    if (verboselevel() > 1) {
      cerr << "shard " << i << " best score: " << best << " variance of the score (for " << option.ntry << " try): " << var << endl;
    }

    totalP += bestP;
    total += best;
    if (verboselevel() > 1)
      cerr << "bestP " << bestP << endl;
  }

  //cerr << "totalP: " << totalP << endl;
  Point finalP = totalP * (1.0 / allTasks.size());
  statscore_t final = total / allTasks.size();

  if (verboselevel() > 1)
    cerr << "bestP: " << finalP << endl;

  // L1-Normalization of the best Point
  if (static_cast<int>(to_optimize.size()) == option.pdim) {
    finalP.NormalizeL1();
  }

  cerr << "Best point: " << finalP << " => " << final << endl;

  if (!WriteFinalWeights(kOutputFile, finalP)) {
    cerr << "Warning: Failed to write the final point" << endl;
  }

  for (size_t i = 0; i < allTasks.size(); ++i) {
    allTasks[i][0]->resetOptimizer();
  }

  PrintUserTime("Stopping...");

  return 0;
}
