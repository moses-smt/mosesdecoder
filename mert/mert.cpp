/**
\description The is the main for the new version of the mert algorithm developed during the 2nd MT marathon
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


float min_interval = 1e-3;

using namespace std;

void usage(void)
{
  cerr<<"usage: mert -d <dimensions> (mandatory )"<<endl;
  cerr<<"[-n] retry ntimes (default 1)"<<endl;
  cerr<<"[-m] number of random directions in powell (default 0)"<<endl;
  cerr<<"[-o] the indexes to optimize(default all)"<<endl;
  cerr<<"[-t] the optimizer(default powell)"<<endl;
  cerr<<"[-r] the random seed (defaults to system clock)"<<endl;
	cerr<<"[-p] only create data for paired ranked optimizer"<<endl;
  cerr<<"[--sctype|-s] the scorer type (default BLEU)"<<endl;
  cerr<<"[--scconfig|-c] configuration string passed to scorer"<<endl;
  cerr<<"[--scfile|-S] comma separated list of scorer data files (default score.data)"<<endl;
  cerr<<"[--ffile|-F] comma separated list of feature data files (default feature.data)"<<endl;
  cerr<<"[--ifile|-i] the starting point data file (default init.opt)"<<endl;
  cerr<<"[-v] verbose level"<<endl;
  cerr<<"[--help|-h] print this message and exit"<<endl;
  exit(1);
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
  {"verbose",1,0,'v'},
  {"help",no_argument,0,'h'},
  {0, 0, 0, 0}
};
int option_index;

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
  string type("powell");
  string scorertype("BLEU");
  string scorerconfig("");
  string scorerfile("statscore.data");
  string featurefile("features.data");
  string initfile("init.opt");
	string pairedrankfile("");

  string tooptimizestr("");
  vector<unsigned> tooptimize;
  vector<vector<parameter_t> > start_list;
  vector<parameter_t> min;
  vector<parameter_t> max;
  //note: those mins and max are the bound for the starting points of the algorithm, not strict bound on the result!

  while ((c=getopt_long (argc, argv, "o:r:d:n:m:t:s:S:F:v:p:", long_options, &option_index)) != -1) {
    switch (c) {
    case 'o':
      tooptimizestr = string(optarg);
      break;
		case 'p':
			pairedrankfile = string(optarg);
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
    default:
      usage();
    }
  }
  if (pdim < 0)
    usage();

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
    std::string substring;
    while (!scorerfile.empty()) {
      getNextPound(scorerfile, substring, ",");
      ScoreDataFiles.push_back(substring);
    }
  }

  vector<string> FeatureDataFiles;
  if (featurefile.length() > 0) {
    std::string substring;
    while (!featurefile.empty()) {
      getNextPound(featurefile, substring, ",");
      FeatureDataFiles.push_back(substring);
    }
  }

  if (ScoreDataFiles.size() != FeatureDataFiles.size()) {
    throw runtime_error("Error: there is a different number of previous score and feature files");
  }

  //it make sense to know what parameter set were used to generate the nbest
  ScorerFactory SF;
  Scorer *TheScorer=SF.getScorer(scorertype,scorerconfig);

  //load data
  Data D(*TheScorer);
  for (size_t i=0; i < ScoreDataFiles.size(); i++) {
    cerr<<"Loading Data from: "<< ScoreDataFiles.at(i)  << " and " << FeatureDataFiles.at(i) << endl;
    D.load(FeatureDataFiles.at(i), ScoreDataFiles.at(i));
  }

  PrintUserTime("Data loaded");

  if (tooptimizestr.length() > 0) {
    cerr << "Weights to optimize: " << tooptimizestr << endl;

    //parse string to get weights to optimize
    //and set them as active
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

	if (pairedrankfile.compare("") != 0) {
		D.sample_ranked_pairs(pairedrankfile);
		PrintUserTime("Stopping...");
		exit(0);
	}

  Optimizer *O=OptimizerFactory::BuildOptimizer(pdim,tooptimize,start_list[0],type,nrandom);
  O->SetScorer(TheScorer);
  O->SetFData(D.getFeatureData());

  // run with specified starting points
  stringstream oss;
  statscore_t best=0, mean=0, var=0;
  Point bestP;
  for(int i=0;i<start_list.size();i++) {
    Point P(start_list[i], min, max);//Generate from the full feature set. Warning: must be done after Optimizer initialization
    statscore_t score=O->Run(P);
    oss.str("");
    oss << "Specified starting point number " << (1+i) << ", score: " << score;
    if (i==0 || score>best) {
      best=score;
      bestP=P;
      oss << " (new best)";
    }
    mean+=score;
    var+=(score*score);
    PrintUserTime(oss.str());
  }

  // run with random starting points
  for(int i=0; i<ntry; i++) {
    Point P(start_list[0], min, max);
    P.Randomize(); // randomize within min and max as given to the constructor
    statscore_t score=O->Run(P);
    oss.str("");
    oss << "Randomized starting point number " << (1+i) << ", score: " << score;
    if(score>best) {
      best=score;
      bestP=P;
      oss << " (new best)";
    }
    mean+=score;
    var+=(score*score);
    PrintUserTime(oss.str());
  }
  mean/=(float)ntry;
  var/=(float)ntry;
  var=sqrt(abs(var-mean*mean));
  if (verboselevel()>1)
    cerr<<"best score: "<< best << " variance of the score (for "<<ntry<<" try): "<<var<<endl;

  // L1-Normalization of the best Point
  if ((int)tooptimize.size() == pdim)
    bestP.NormalizeL1();

  cerr << "Best point: " << bestP << " => " << best << endl;
  ofstream res("weights.txt");
  res<<bestP<<endl;

  PrintUserTime("Stopping...");
}
