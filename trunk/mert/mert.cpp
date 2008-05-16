/**
\description The is the main for the new version of the mert algorithm develloppped during the 2nd MT marathon
*/

#include <limits>
#include "Data.h"
#include "Point.h"
#include "Scorer.h"
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
#include "Util.h"


float min_interval = 1e-3;

using namespace std;

void usage(void) {
  cerr<<"usage: mert -d <dimensions> (mandatory )"<<endl;
  cerr<<"[-n retry ntimes (default 1)]"<<endl;
  cerr<<"[-o\tthe indexes to optimize(default all)]"<<endl;
  cerr<<"[-t\tthe optimizer(default Powell)]"<<endl;
  cerr<<"[--sctype] the scorer type (default BLEU)"<<endl;
  cerr<<"[--scfile] the scorer data file (default score.data)"<<endl;
  cerr<<"[--ffile] the feature data file data file (default feature.data)"<<endl;
  cerr<<"[-v] verbose level";
  exit(1);
}

static struct option long_options[] =
  {
    {"dim", 1, 0, 'd'},
    {"ntry",1,0,'n'},
    {"optimize",1,0,'o'},
    {"type",1,0,'t'},
    {"sctype",1,0,'s'},
    {"scfile",1,0,'S'},
    {"ffile",1,0,'F'},
    {"verbose",1,0,'v'},
    {0, 0, 0, 0}
  };
int option_index;

int main (int argc, char **argv) {
  int c,dim,i;
  dim=-1;
  int ntry=1;
  string type("powell");
  string scorertype("BLEU");
  string scorerfile("statscore.data");
  string featurefile("features.data");
  vector<unsigned> tooptimize;
  vector<parameter_t> start;
  while ((c=getopt_long (argc, argv, "d:n:t:s:S:F:v:", long_options, &option_index)) != -1) {
    switch (c) {
    case 'd':
      dim = strtol(optarg, NULL, 10);
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
    case 'F':
      featurefile=string(optarg);
      break;
    case 'v':
      setverboselevel(strtol(optarg,NULL,10));
      break;
    default:
      usage();
    }
  }
  if (dim < 0)
    usage();
  if(tooptimize.empty()){//We'll optimize on everything
    tooptimize.resize(dim);
    for(i=0;i<dim;i++)
      tooptimize[i]=i;
  }
  ScorerFactory SF;
  Optimizer *O;
  Scorer *TheScorer=NULL;;
  FeatureData *FD=NULL;
;
  start.resize(dim);
  float score;
  float best=numeric_limits<float>::min();
  float mean=0;
  float var=0;
  Point bestP;
  //it make sense to know what parameter set where used to generate the nbest
  O=BuildOptimizer(dim,tooptimize,start,"powell");
  
  TheScorer=SF.getScorer(scorertype);
  ScoreData *SD=new ScoreData(*TheScorer);
  FD=new FeatureData();
  FD->load(featurefile);
  SD->load(scorerfile);
  O->SetScorer(TheScorer);
  O->SetFData(FD);
  Point min;//to: initialize
  Point max;
  //note: thos min and maw are the bound for the starting poitns of the algorithm, not strict bound on the result!
  for(int d=0;d<Point::getdim();d++){
    min[d]=0.0;
    max[d]=1.0;
  }
  for(int i=0;i<ntry;i++){
    Point P;
    P.Randomize(min,max);
    score=O->Run(P);
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
  cerr<<"variance of the score (for "<<ntry<<" try):"<<var<<endl;
  cerr<<"best score"<<best<<endl;
  ofstream res("weights.txt");
  res<<bestP<<endl;
}
