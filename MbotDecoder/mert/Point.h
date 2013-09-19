#ifndef POINT_H
#define POINT_H

#include <fstream>
#include <map>
#include <vector>
#include "Types.h"

class FeatureStats;
class Optimizer;

/**
 * A class that handles the subset of the Feature weight on which
 * we run the optimization.
 */
class Point : public vector<parameter_t>
{
  friend class Optimizer;
private:
  /**
   * The indices over which we optimize.
   */
  static vector<unsigned int> optindices;

  /**
   * Dimension of optindices and of the parent vector.
   */
  static unsigned int dim;

  /**
   * Fixed weights in case of partial optimzation.
   */
  static map<unsigned int,parameter_t> fixedweights;

  /**
   * Total size of the parameter space; we have
   * pdim = FixedWeight.size() + optinidices.size().
   */
  static unsigned int pdim;
  static unsigned int ncall;

  /**
   * The limits for randomization, both vectors are of full length, pdim.
   */
  static vector<parameter_t> m_min;
  static vector<parameter_t> m_max;

  statscore_t score_;

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
  }

  Point();
  Point(const vector<parameter_t>& init,
        const vector<parameter_t>& min,
        const vector<parameter_t>& max);
  ~Point();

  void Randomize();

  // Compute the feature function
  double operator*(const FeatureStats&) const;
  Point operator+(const Point&) const;
  void operator+=(const Point&);
  Point operator*(float) const;

  /**
   * Write the Whole featureweight to a stream (ie pdim float).
   */
  friend ostream& operator<<(ostream& o,const Point& P);

  void Normalize() { NormalizeL2(); }
  void NormalizeL2();
  void NormalizeL1();

  /**
   * Return a vector of size pdim where all weights have been
   * put (including fixed ones).
   */
  vector<parameter_t> GetAllWeights() const;

  statscore_t GetScore() const {
    return score_;
  }

  void SetScore(statscore_t score) { score_ = score; }
};

#endif  // POINT_H
