#ifndef OPTIMIZER_H
#define OPTIMIZER_H
#include <vector>
#include "FeatureStats.h"
#include "FeatureData.h"
#include "FeatureArray.h"
#include "Scorer.h"
#include "Point.h"
#include "Types.h"


typedef float featurescore;



using namespace std;
/**abstract virtual class*/
class Optimizer{
 protected:
   Scorer * scorer; //no accessor for them only child can use them 
   FeatureData * FData;//no accessor for them only child can use them 
 public:
  Optimizer(unsigned Pd,vector<unsigned> i2O,vector<parameter_t> start);
  void SetScorer(Scorer *S);
  void SetFData(FeatureData *F);
  virtual ~Optimizer();

  unsigned size()const{return (FData?FData->size():0);}
  /**Generic wrapper around TrueRun to check a few things. Non virtual*/
  statscore_t  Run(Point&)const;
/**main function that perform an optimization*/  
  virtual  statscore_t  TrueRun(Point&)const=0;
  /**given a set of lambdas, get the nbest for each sentence*/
  void Get1bests(const Point& param,vector<unsigned>& bests)const;
  /**given a set of nbests, get the Statistical score*/
  statscore_t  GetStatScore(const vector<unsigned>& nbests)const{return scorer->score(nbests);};
  /**given a set of lambdas, get the total statistical score*/
  statscore_t  GetStatScore(const Point& param)const;  
  vector<statscore_t > GetIncStatScore(vector<unsigned> ref,vector<vector <pair<unsigned,unsigned> > >)const;
  statscore_t  LineOptimize(const Point& start,const Point& direction,Point& best)const;//Get the optimal Lambda and the best score in a particular direction from a given Point
};


/**default basic  optimizer*/
class SimpleOptimizer: public Optimizer{
private:
static  float eps;
public:
  SimpleOptimizer(unsigned dim,vector<unsigned> i2O,vector<parameter_t> start):Optimizer(dim,i2O,start){};
  virtual statscore_t  TrueRun(Point&)const;
};

class RandomOptimizer: public Optimizer{
public:
  RandomOptimizer(unsigned dim,vector<unsigned> i2O,vector<parameter_t> start):Optimizer(dim,i2O,start){};
  virtual statscore_t  TrueRun(Point&)const;
};



class OptimizerFactory{
 public:
  // unsigned dim;
  //Point Start;
  static vector<string> GetTypeNames();
  static Optimizer* BuildOptimizer(unsigned dim,vector<unsigned>tooptimize,vector<parameter_t> start,string type);
 private:
  enum OptType{POWELL=0,RANDOM,NOPTIMIZER};//Add new optimizer here BEFORE NOPTIMZER    
  static OptType GetOType(string);
  static vector<string> typenames;
  static void SetTypeNames();
  
};


#endif

