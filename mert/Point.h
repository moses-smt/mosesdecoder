#ifndef POINT_H
#define POINT_H
#include <vector>
#include "FeatureStats.h"
#include <cassert>
typedef float lambda,statscore;


class Optimizer;

/**class that handle the subset of the Feature weight on which we run the optimization*/

class Point:public vector<lambda>{
  friend class Optimizer;
 private:
  /**The indices over which we optimize*/
  static vector<unsigned> optindices;
  /**dimension of optindices and of the parent vector*/
  static unsigned dim;
  /**fixed weights in case of partial optimzation*/
  static map<unsigned,lambda> fixedweights;
  /**total size of the parameter space; we have pdim=FixedWeight.size()+optinidices.size()*/
  static unsigned pdim;
 public:
  static unsigned getdim(){return dim;}
  static unsigned getpdim(){return pdim;}
  static bool OptimizeAll(){return fixedweights.empty();};
  statscore score;
  Point():vector<lambda>(dim){};
  Point(vector<lambda> init):vector<lambda>(init){assert(init.size()==dim);};
  void Randomize(const std::vector<lambda>&min,const std::vector<lambda>& max);
  double operator*(const FeatureStats&)const;//compute the feature function
  Point operator+(const Point&)const;
  Point operator*(float)const;
  /**write the Whole featureweight to a stream (ie pdim float)*/
  friend ostream& operator<<(ostream& o,const Point& P);
  void Normalize();
  /**return a vector of size pdim where all weights have been put*/
  vector<lambda> GetAllWeights()const;
};

#endif

