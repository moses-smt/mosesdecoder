#ifndef OPTIMIZER_H
#define OPTIMIZER_H
#include<vector>
#include "FeatureStats.h"
#include "FeatureData.h"
#include "FeatureArray.h"
#include "scorer.h"
#include "point.h"



typedef float featurescore;




using namespace std;
/**virtual class*/
class Optimizer{
 public:
   Scorer * scorer; 
   FeatureData * FData; 
   /**number of lambda parameters*/ 
  unsigned dimension;
  Optimizer(unsigned d):dimension(d),scorer(NULL),FData(NULL){};
  void SetScorer(Scorer *S);
  void SetFData(FeatureData *F);
  ~Optimizer(){
    delete scorer;
    delete FData;
  }
  /**Number of sentences in the tuning set*/
  unsigned N;
  /**main function that perform an optimization*/
  virtual  point run(const point& init);
  /**given a set of lambdas, get the nbest for each sentence*/
  vector<unsigned> Get1bests(const point& param);
  /**given a set of nbests, get the Statistical score*/
  statscore Getstatscore(vector<unsigned> nbests){scorer->score(nbests);};
  /**given a set of lambdas, get the total statistical score*/
  statscore Getstatscore(const point& param){return Getstatscore(Get1bests(param));};
  statscore LineOptimize(const point& start,point direction,point& best);//Get the optimal Lambda and the best score in a particular direction from a given point
}

using namespace std;
/**default basic  optimizer*/
class SimpleOptimizer: public Optimizer{
private: float eps;
public:
  SimpleOptimizer(unsigned dim,float _eps):Optimizer(dim),eps(_eps){};
  point run(const point& init);
}

#endif

