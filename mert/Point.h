#ifndef POINT_H
#define POINT_H
#include <vector>
#include "Types.h"
#include "FeatureStats.h"
#include <cassert>


class Optimizer;

/**class that handle the subset of the Feature weight on which we run the optimization*/

class Point:public parameters_t{
  friend class Optimizer;
 private:
  /**The indices over which we optimize*/
  static vector<unsigned> optindices;
  /**dimension of optindices and of the parent vector*/
  static unsigned dim;
  /**fixed weights in case of partial optimzation*/
  static map<unsigned,parameter_t> fixedweights;
  /**total size of the parameter space; we have pdim=FixedWeight.size()+optinidices.size()*/
  static unsigned pdim;
  static unsigned ncall;
 public:
  static unsigned getdim(){return dim;}
  static unsigned getpdim(){return pdim;}
  static bool OptimizeAll(){return fixedweights.empty();};
  statscore_t score;
  Point():parameters_t(dim){};
  Point(parameters_t init):parameters_t(init){assert(init.size()==dim);};
  void Randomize(const parameters_t& min,const parameters_t& max);

  double operator*(const FeatureStats&)const;//compute the feature function
  Point operator+(const Point&)const;
  Point operator*(float)const;
  /**write the Whole featureweight to a stream (ie pdim float)*/
  friend ostream& operator<<(ostream& o,const Point& P);
  void Normalize();
  /**return a vector of size pdim where all weights have been put*/
  parameters_t GetAllWeights()const;
};

#endif

