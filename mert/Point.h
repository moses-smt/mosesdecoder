#ifndef POINT_H
#define POINT_H

#include <vector>
#include "Types.h"
#include "FeatureStats.h"
#include <cassert>


class Optimizer;

/**class that handle the subset of the Feature weight on which we run the optimization*/

class Point:public vector<parameter_t>
{
  friend class Optimizer;
private:
  /**The indices over which we optimize*/
  static vector<unsigned int> optindices;
  /**dimension of optindices and of the parent vector*/
  static unsigned int dim;
  /**fixed weights in case of partial optimzation*/
  static map<unsigned int,parameter_t> fixedweights;
  /**total size of the parameter space; we have pdim=FixedWeight.size()+optinidices.size()*/
  static unsigned int pdim;
  static unsigned int ncall;
  /**The limits for randomization, both vectors are of full length, pdim*/
  static vector<parameter_t> m_min;
  static vector<parameter_t> m_max;
public:
  static unsigned int getdim() {
    return dim;
  }
  static unsigned int getpdim() {
    return pdim;
  }
  static void setpdim(size_t pd) {
    pdim = pd;
  }
  static void setdim(size_t d) {
    dim = d;
  }
  static bool OptimizeAll() {
    return fixedweights.empty();
  };
  statscore_t score;
  Point():vector<parameter_t>(dim) {};
  Point(const vector<parameter_t>& init,
    const vector<parameter_t>& min,
    const vector<parameter_t>& max
  );
  void Randomize();

  double operator*(const FeatureStats&)const;//compute the feature function
  Point operator+(const Point&)const;
  void operator+=(const Point&);
  Point operator*(float)const;
  /**write the Whole featureweight to a stream (ie pdim float)*/
  friend ostream& operator<<(ostream& o,const Point& P);
  void Normalize() {
    NormalizeL2();
  };
  void NormalizeL2();
  void NormalizeL1();
  /**return a vector of size pdim where all weights have been put(including fixed ones)*/
  vector<parameter_t> GetAllWeights()const;
  statscore_t GetScore()const {
    return score;
  };
};

#endif  // POINT_H
