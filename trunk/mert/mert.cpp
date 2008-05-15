/**
\description The is the main for the new versionh of the mert algorithm develloppped during the 2nd MT marathon
*/

#include <limits>
#include "Data.h"
#include "Point.h"
#include "Scorer.h"
#include "ScoreData.h"
#include "FeatureData.h"
#include "Optimizer.h"
#include "getopt.h"
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cmath>

int verbose = 2;

float min_interval = 1e-3;

using namespace std;

void usage(void) {
  cerr<<"usage: mert -d <dimensions> \n[-n] retry ntimes\n";
  exit(1);
}

static struct option long_options[] =
  {
    {"dim", 1, 0, 'd'},
    {"only",1,0,'o'},
    {"type",1,0,'t'},
    {"scorer",1,0,'s'},
    {0, 0, 0, 0}
  };
int option_index;

int main (int argc, char **argv) {
  int c,dim,i;
  dim=-1;
  int ntry=1;
  string type("powell");
  string scorertype("BLEU");
  vector<unsigned> tooptimize;
  vector<lambda> start;
  while (getopt_long (argc, argv, "d:n:o:t:s:", long_options, &option_index) != -1) {
    switch (c) {
    case 'd':
      dim = strtol(optarg, NULL, 10);
      break;
    case 'n':
      ntry=strtol(optarg, NULL, 10);
      break;
    case 'o':
      //TODO
    case 't':
      type=string(optarg);
      case's':
	scorertype=string(optarg);
    default:
      usage();
    }
  }
  if(tooptimize.empty()){//We'll optimize on everything
    tooptimize.resize(dim);
    for(i=0;i<dim;i++)
      tooptimize[i]=i;
  }
  Optimizer *O;
  Scorer *TheScorer=NULL;;
  FeatureData *FD=NULL;
  if (dim < 0)
    usage();
  start.resize(dim);
  float score;
  float best=numeric_limits<float>::max();
  float mean=0;
  float var=0;
  Point bestP;
  //it make sense to know what parameter set where used to generate the nbest
  O=BuildOptimizer(dim,tooptimize,start,"powell");
  O->SetScorer(TheScorer);
  O->SetFData(FD);
  Point min;//to: initialize
  Point max;
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
  var=sqrt(var);
  cerr<<"variance of the score(for "<<ntry<<" try):"<<var<<endl;
  cerr<<"best score"<<best<<endl;
  ofstream res("weights.txt");
  res<<bestP<<endl;
}
