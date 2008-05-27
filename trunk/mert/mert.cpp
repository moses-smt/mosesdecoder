/**
\description The is the main for the new version of the mert algorithm develloppped during the 2nd MT marathon
*/

#include <limits>
#include "Data.h"
#include "Point.h"
#include "Scorer.h"
#include "ScorerFactory.h"
#include "ScoreData.h"
#include "FeatureData.h"
#include "Optimizer.h"
#include "getopt.h"
#include "Types.h"
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>
#include "Timer.h"
#include "Util.h"


float min_interval = 1e-3;

using namespace std;

void usage(void) {
  cerr<<"usage: mert -d <dimensions> (mandatory )"<<endl;
  cerr<<"[-n retry ntimes (default 1)]"<<endl;
  cerr<<"[-o\tthe indexes to optimize(default all)]"<<endl;
  cerr<<"[-t\tthe optimizer(default powell)]"<<endl;
  cerr<<"[--sctype|-s] the scorer type (default BLEU)"<<endl;
  cerr<<"[--scfile|-S] the scorer data file (default score.data)"<<endl;
  cerr<<"[--ffile|-F] the feature data file (default feature.data)"<<endl;
	cerr<<"[--ifile|-i] the starting point data file (default init.opt)"<<endl;
	cerr<<"[-v] verbose level"<<endl;
  exit(1);
}

static struct option long_options[] =
  {
    {"pdim", 1, 0, 'd'},
    {"ntry",1,0,'n'},
    {"optimize",1,0,'o'},
    {"type",1,0,'t'},
    {"sctype",1,0,'s'},
    {"scfile",1,0,'S'},
    {"ffile",1,0,'F'},
    {"ifile",1,0,'i'},
    {"verbose",1,0,'v'},
    {0, 0, 0, 0}
  };
int option_index;

int main (int argc, char **argv) {
  int c,pdim,i;
  pdim=-1;
  int ntry=1;
  string type("powell");
  string scorertype("BLEU");
  string scorerfile("statscore.data");
  string featurefile("features.data");
  string initfile("init.opt");
	vector<unsigned> tooptimize;
  vector<parameter_t> start;
  while ((c=getopt_long (argc, argv, "d:n:t:s:S:F:v:", long_options, &option_index)) != -1) {
    switch (c) {
    case 'd':
      pdim = strtol(optarg, NULL, 10);
      break;
    case 'n':
      ntry=strtol(optarg, NULL, 10);
      break;
    case 't':
      type=string(optarg);
      break;
    case's':
      scorertype=string(optarg);
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

  Timer timer;
  timer.start("Starting...");
  
  if(tooptimize.empty()){
    tooptimize.resize(pdim);//We'll optimize on everything
    for(i=0;i<pdim;i++)
      tooptimize[i]=i;
  }
  ifstream opt(initfile.c_str());
  if(opt.fail()){
    cerr<<"could not open initfile: " << initfile << endl;
    exit(3);
  }
  start.resize(pdim);//to do:read from file
  int j;
  for( j=0;j<pdim&&!opt.fail();j++)
    opt>>start[j];
  if(j<pdim){
    cerr<<"error could not initialize start point with " << initfile << endl;
    exit(3);
  }

  opt.close();
  //it make sense to know what parameter set were used to generate the nbest
  ScorerFactory SF;
  Scorer *TheScorer=SF.getScorer(scorertype);

  cerr<<"Loading Data from: "<< scorerfile  << " and " << featurefile << endl;
  Data D(*TheScorer);
  D.load(featurefile, scorerfile);

  Optimizer *O=OptimizerFactory::BuildOptimizer(pdim,tooptimize,start,type);
  O->SetScorer(TheScorer);
  O->SetFData(D.getFeatureData());
  Point P(start);//Generate from the full feature set. Warning: must be done after Optimizer initialization
  statscore_t best=O->Run(P);
  Point bestP=P;  
  statscore_t mean=best;
  statscore_t var=best*best;
   
  vector<parameter_t> min(Point::getdim());
  vector<parameter_t> max(Point::getdim());
 
 for(unsigned int d=0;d<Point::getdim();d++){
    min[d]=0.0;
    max[d]=1.0;
  }
  //note: those mins and max are the bound for the starting points of the algorithm, not strict bound on the result!
  
 for(int i=1;i<ntry;i++){
   P.Randomize(min,max);
   statscore_t score=O->Run(P);
   if(score>best){
     best=score;
     bestP=P;
  }
   mean+=score;
   var+=(score*score);
 }
 mean/=(float)ntry;
 var/=(float)ntry;
 var=sqrt(abs(var-mean*mean));
 if (verboselevel()>1)
	 cerr<<"best score: "<< best << " variance of the score (for "<<ntry<<" try): "<<var<<endl;

 //L1-Normalization of the best Point
 bestP.NormalizeL1();
 
 cerr << "Best point: " << bestP << " => " << best << endl;
 ofstream res("weights.txt");
 res<<bestP<<endl;
 
 timer.stop("Stopping...");
}
