#ifndef OPTIMIZER_H
#define OPTIMIZER_H
#include <vector>
#include "FeatureStats.h"
#include "FeatureData.h"
#include "FeatureArray.h"
#include "Scorer.h"
#include "Point.h"



typedef float featurescore;



using namespace std;
/**abstract virtual class*/
class Optimizer{
 protected:
   Scorer * scorer; //no accessor for them only child can use them 
   FeatureData * FData;//no accessor for them only child can use them 
 public:
  Optimizer(unsigned Pd,vector<unsigned> i2O,vector<lambda> start);
  void SetScorer(Scorer *S);
  void SetFData(FeatureData *F);
  virtual ~Optimizer();

  unsigned size()const{return (FData?FData->size():0);}
  /**Generic wrapper around TrueRun to check a few things. Non virtual*/
  statscore Run(Point&)const;
/**main function that perform an optimization*/  
  virtual  statscore TrueRun(Point&)const=0;
  /**given a set of lambdas, get the nbest for each sentence*/
  void Get1bests(const Point& param,vector<unsigned>& bests)const;
  /**given a set of nbests, get the Statistical score*/
  statscore GetStatScore(const vector<unsigned>& nbests)const{scorer->score(nbests);};
  /**given a set of lambdas, get the total statistical score*/
  statscore GetStatScore(const Point& param)const;  
  vector<statscore> GetIncStatScore(vector<unsigned> ref,vector<vector <pair<unsigned,unsigned> > >)const;
  statscore LineOptimize(const Point& start,const Point& direction,Point& best)const;//Get the optimal Lambda and the best score in a particular direction from a given Point
};


/**default basic  optimizer*/
class SimpleOptimizer: public Optimizer{
private: float eps;
public:
  SimpleOptimizer(unsigned dim,vector<unsigned> i2O,vector<lambda> start):Optimizer(dim,i2O,start),eps(0.001){};
  virtual statscore TrueRun(Point&)const;
};

Optimizer *BuildOptimizer(unsigned dim,vector<unsigned>tooptimize,vector<lambda>start,string type);

#endif

