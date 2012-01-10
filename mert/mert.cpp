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

#include "Data.h"
#include "Point.h"
#include "Scorer.h"
#include "ScorerFactory.h"
#include "ScoreData.h"
#include "FeatureData.h"
#include "Optimizer.h"
#include "Types.h"
#include "Timer.h"
#include "Util.h"

#include "../moses/src/ThreadPool.h"


float min_interval = 1e-3;

using namespace std;

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
  {"pro",required_argument,0,'p'},
  {"type",1,0,'t'},
  {"sctype",1,0,'s'},
  {"scconfig",required_argument,0,'c'},
  {"scfile",1,0,'S'},
  {"ffile",1,0,'F'},
  {"ifile",1,0,'i'},
#ifdef WITH_THREADS
  {"threads", required_argument,0,'T'},
#endif
  {"shard-count", required_argument, 0, 'a'},
  {"shard-size", required_argument, 0, 'b'},
  {"verbose",1,0,'v'},
  {"help",no_argument,0,'h'},
  {0, 0, 0, 0}
};
int option_index;

/**
 * Runs an optimisation, or a random restart.
 */
class OptimizationTask : public Moses::Task
{
 public:
  OptimizationTask(Optimizer* optimizer, const Point& point) :
      m_optimizer(optimizer), m_point(point) {}

  ~OptimizationTask() {}

  void resetOptimizer() {
    if (m_optimizer) {
      delete m_optimizer;
      m_optimizer = NULL;
    }
  }

  bool DeleteAfterExecution() {
    return false;
  }

  void Run() {
    m_score = m_optimizer->Run(m_point);
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

int main (int argc, char **argv)
{
  ResetUserTime();

  /*
   Timer timer;
   timer.start("Starting...");
  */

  int c,pdim,i;
  pdim=-1;
  int ntry=1;
  int nrandom=0;
  int seed=0;
  bool hasSeed = false;
#ifdef WITH_THREADS
  size_t threads=1;
#endif
  float shard_size = 0;
  size_t shard_count = 0;
  string type("powell");
  string scorertype("BLEU");
  string scorerconfig("");
  string scorerfile("statscore.data");
  string featurefile("features.data");
  string initfile("init.opt");

  string tooptimizestr("");
  vector<unsigned> tooptimize;
  vector<vector<parameter_t> > start_list;
  vector<parameter_t> min;
  vector<parameter_t> max;
  // NOTE: those mins and max are the bound for the starting points of the algorithm, not strict bound on the result!

  while ((c=getopt_long (argc, argv, "o:r:d:n:m:t:s:S:F:v:p:", long_options, &option_index)) != -1) {
    switch (c) {
      case 'o':
        tooptimizestr = string(optarg);
        break;
      case 'd':
        pdim = strtol(optarg, NULL, 10);
        break;
      case 'n':
        ntry=strtol(optarg, NULL, 10);
        break;
      case 'm':
        nrandom=strtol(optarg, NULL, 10);
        break;
      case 'r':
        seed=strtol(optarg, NULL, 10);
        hasSeed = true;
        break;
      case 't':
        type=string(optarg);
        break;
      case's':
        scorertype=string(optarg);
        break;
      case 'c':
        scorerconfig = string(optarg);
        break;
      case 'S':
        scorerfile=string(optarg);
        break;
      case 'F':
        featurefile=string(optarg);
        break;
      case 'i':
        initfile=string(optarg);
        break;
      case 'v':
        setverboselevel(strtol(optarg,NULL,10));
        break;
#ifdef WITH_THREADS
      case 'T':
        threads = strtol(optarg, NULL, 10);
        if (threads < 1) threads = 1;
        break;
#endif
      case 'a':
        shard_count = strtof(optarg,NULL);
        break;
      case 'b':
        shard_size = strtof(optarg,NULL);
        break;
      case 'h':
        usage(0);
        break;
      default:
        usage(1);
    }
  }
  if (pdim < 0)
    usage(1);

  cerr << "shard_size = " << shard_size << " shard_count = " << shard_count << endl;
  if (shard_size && !shard_count) {
    cerr << "Error: shard-size provided without shard-count" << endl;
    exit(1);
  }
  if (shard_size > 1 || shard_size < 0) {
    cerr << "Error: shard-size should be between 0 and 1" << endl;
    exit(1);
  }

  if (hasSeed) {
    cerr << "Seeding random numbers with " << seed << endl;
    srandom(seed);
  } else {
    cerr << "Seeding random numbers with system clock " << endl;
    srandom(time(NULL));
  }

  // read in starting points
  std::string onefile;
  while (!initfile.empty()) {
    getNextPound(initfile, onefile, ",");
    vector<parameter_t> start;
    ifstream opt(onefile.c_str());
    if(opt.fail()) {
      cerr<<"could not open initfile: " << initfile << endl;
      exit(3);
    }
    start.resize(pdim);//to do:read from file
    int j;
    for( j=0; j<pdim&&!opt.fail(); j++)
      opt>>start[j];
    if(j<pdim) {
      cerr<<initfile<<":Too few starting weights." << endl;
      exit(3);
    }
    start_list.push_back(start);
    // for the first time, also read in the min/max values for scores
    if (start_list.size() == 1) {
      min.resize(pdim);
      for( j=0; j<pdim&&!opt.fail(); j++)
        opt>>min[j];
      if(j<pdim) {
        cerr<<initfile<<":Too few minimum weights." << endl;
        cerr<<"error could not initialize start point with " << initfile << endl;
	std::cerr << "j: " << j << ", pdim: " << pdim << std::endl;
        exit(3);
      }
      max.resize(pdim);
      for( j=0; j<pdim&&!opt.fail(); j++)
        opt>>max[j];
      if(j<pdim) {
        cerr<<initfile<<":Too few maximum weights." << endl;
        exit(3);
      }
    }
    opt.close();
  }

  vector<string> ScoreDataFiles;
  if (scorerfile.length() > 0) {
    Tokenize(scorerfile.c_str(), ',', &ScoreDataFiles);
  }

  vector<string> FeatureDataFiles;
  if (featurefile.length() > 0) {
    Tokenize(featurefile.c_str(), ',', &FeatureDataFiles);
  }

  if (ScoreDataFiles.size() != FeatureDataFiles.size()) {
    throw runtime_error("Error: there is a different number of previous score and feature files");
  }

  // it make sense to know what parameter set were used to generate the nbest
  Scorer *TheScorer = ScorerFactory::getScorer(scorertype,scorerconfig);

  //load data
  Data D(*TheScorer);
  for (size_t i=0; i < ScoreDataFiles.size(); i++) {
    cerr<<"Loading Data from: "<< ScoreDataFiles.at(i)  << " and " << FeatureDataFiles.at(i) << endl;
    D.load(FeatureDataFiles.at(i), ScoreDataFiles.at(i));
  }

  //ADDED_BY_TS
  D.remove_duplicates();
  //END_ADDED

  PrintUserTime("Data loaded");

  // starting point score over latest n-best, accumulative n-best
  //vector<unsigned> bests;
  //compute bests with sparse features needs to be implemented
  //currently sparse weights are not even loaded
  //statscore_t score = TheScorer->score(bests);

  if (tooptimizestr.length() > 0) {
    cerr << "Weights to optimize: " << tooptimizestr << endl;

    // Parse string to get weights to optimize, and set them as active
    std::string substring;
    int index;
    while (!tooptimizestr.empty()) {
      getNextPound(tooptimizestr, substring, ",");
      index = D.getFeatureIndex(substring);
      cerr << "FeatNameIndex:" << index << " to insert" << endl;
      //index = strtol(substring.c_str(), NULL, 10);
      if (index >= 0 && index < pdim) {
        tooptimize.push_back(index);
      } else {
        cerr << "Index " << index << " is out of bounds. Allowed indexes are [0," << (pdim-1) << "]." << endl;
      }
    }
  } else {
    //set all weights as active
    tooptimize.resize(pdim);//We'll optimize on everything
    for(int  i=0; i<pdim; i++) {
      tooptimize[i]=1;
    }
  }

  // treat sparse features just like regular features
  if (D.hasSparseFeatures()) {
    D.mergeSparseFeatures();
  }


#ifdef WITH_THREADS
  cerr << "Creating a pool of " << threads << " threads" << endl;
  Moses::ThreadPool pool(threads);
#endif

  Point::setpdim(pdim);
  Point::setdim(tooptimize.size());

  //starting points consist of specified points and random restarts
  vector<Point> startingPoints;

  for (size_t i = 0; i < start_list.size(); ++i) {
    startingPoints.push_back(Point(start_list[i],min,max));
  }
  for (int i = 0; i < ntry; ++i) {
    startingPoints.push_back(Point(start_list[0],min,max));
    startingPoints.back().Randomize();
  }


  vector<vector<OptimizationTask*> > allTasks(1);

  //optional sharding
  vector<Data> shards;
  if (shard_count) {
    D.createShards(shard_count, shard_size, scorerconfig, shards);
    allTasks.resize(shard_count);
  }

  // launch tasks
  for (size_t i = 0 ; i < allTasks.size(); ++i) {
    Data& data = D;
    if (shard_count) data = shards[i]; //use the sharded data if it exists
    vector<OptimizationTask*>& tasks = allTasks[i];
    Optimizer *O = OptimizerFactory::BuildOptimizer(pdim,tooptimize,start_list[0],type,nrandom);
    O->SetScorer(data.getScorer());
    O->SetFData(data.getFeatureData());
    //A task for each start point
    for (size_t j = 0; j < startingPoints.size(); ++j) {
      OptimizationTask* task = new OptimizationTask(O, startingPoints[j]);
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
    statscore_t best=0, mean=0, var=0;
    Point bestP;
    for (size_t j = 0; j < allTasks[i].size(); ++j) {
      statscore_t score = allTasks[i][j]->getScore();
      mean += score;
      var += score*score;
      if (score > best) {
        bestP = allTasks[i][j]->getPoint();
        best = score;
      }
    }

    mean/=(float)ntry;
    var/=(float)ntry;
    var=sqrt(abs(var-mean*mean));
    if (verboselevel()>1)
      cerr<<"shard " << i << " best score: "<< best << " variance of the score (for "<<ntry<<" try): "<<var<<endl;

    totalP += bestP;
    total += best;
    if (verboselevel()>1)
      cerr << "bestP " << bestP << endl;
  }

  //cerr << "totalP: " << totalP << endl;
  Point finalP = totalP * (1.0 / allTasks.size());
  statscore_t final = total / allTasks.size();

  if (verboselevel()>1)
    cerr << "bestP: " << finalP << endl;

  // L1-Normalization of the best Point
  if ((int)tooptimize.size() == pdim)
    finalP.NormalizeL1();

  cerr << "Best point: " << finalP << " => " << final << endl;
  ofstream res("weights.txt");
  res<<finalP<<endl;

  for (size_t i = 0; i < allTasks.size(); ++i) {
    allTasks[i][0]->resetOptimizer();
    for (size_t j = 0; j < allTasks[i].size(); ++j) {
      delete allTasks[i][j];
    }
  }

  delete TheScorer;
  PrintUserTime("Stopping...");
}
