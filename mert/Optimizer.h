#ifndef OPTIMIZER_H
#define OPTIMIZER_H
#include<vector>
#include "FeatureStats.h"
#include "FeatureData.h"
#include "FeatureArray.h"
#include "Scorer.h"
#include "Point.h"



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
  virtual  Point run(const Point& init);
  /**given a set of lambdas, get the nbest for each sentence*/
  vector<unsigned> Get1bests(const Point& param);
  /**given a set of nbests, get the Statistical score*/
  statscore Getstatscore(vector<unsigned> nbests){scorer->score(nbests);};
  /**given a set of lambdas, get the total statistical score*/
  statscore Getstatscore(const Point& param){return Getstatscore(Get1bests(param));};
  statscore LineOptimize(const Point& start,Point direction,Point& best);//Get the optimal Lambda and the best score in a particular direction from a given Point
}

using namespace std;
/**default basic  optimizer*/
class SimpleOptimizer: public Optimizer{
private: float eps;
public:
  SimpleOptimizer(unsigned dim,float _eps):Optimizer(dim),eps(_eps){};
  Point run(const Point& init);
}

#endif

