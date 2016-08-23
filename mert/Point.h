#ifndef MERT_POINT_H_
#define MERT_POINT_H_

#include <ostream>
#include <map>
#include <vector>
#include "Types.h"

namespace MosesTuning
{


class FeatureStats;
class Optimizer;

/**
 * A class that handles the subset of the Feature weight on which
 * we run the optimization.
 */
class Point : public std::vector<parameter_t>
{
  friend class Optimizer;

private:
  /**
   * The indices over which we optimize.
   */
  static std::vector<unsigned int> m_opt_indices;

  /**
   * Dimension of m_opt_indices and of the parent vector.
   */
  static unsigned int m_dim;

  /**
   * Fixed weights in case of partial optimzation.
   */
  static std::map<unsigned int,parameter_t> m_fixed_weights;

  /**
   * Total size of the parameter space; we have
   * m_pdim = FixedWeight.size() + optinidices.size().
   */
  static unsigned int m_pdim;
  static unsigned int m_ncall;

  /**
   * The limits for randomization, both vectors are of full length, m_pdim.
   */
  static std::vector<parameter_t> m_min;
  static std::vector<parameter_t> m_max;

  statscore_t m_score;

public:
  static unsigned int getdim() {
    return m_dim;
  }
  static void setdim(std::size_t d) {
    m_dim = d;
  }

  static unsigned int getpdim() {
    return m_pdim;
  }
  static void setpdim(std::size_t pd) {
    m_pdim = pd;
  }

  static void set_optindices(const std::vector<unsigned int>& indices) {
    m_opt_indices = indices;
  }

  static const std::vector<unsigned int>& get_optindices() {
    return m_opt_indices;
  }

  static bool OptimizeAll() {
    return m_fixed_weights.empty();
  }

  Point();
  Point(const std::vector<parameter_t>& init,
        const std::vector<parameter_t>& min,
        const std::vector<parameter_t>& max);
  ~Point();

  void Randomize();

  // Compute the feature function
  double operator*(const FeatureStats&) const;
  const Point operator+(const Point&) const;
  void operator+=(const Point&);
  const Point operator*(float) const;

  /**
   * Write the Whole featureweight to a stream (ie m_pdim float).
   */
  friend std::ostream& operator<<(std::ostream& o,const Point& P);

  void Normalize() {
    NormalizeL2();
  }
  void NormalizeL2();
  void NormalizeL1();

  /**
   * Return a vector of size m_pdim where all weights have been
   * put (including fixed ones).
   */
  void GetAllWeights(std::vector<parameter_t>& w) const;

  statscore_t GetScore() const {
    return m_score;
  }
  void SetScore(statscore_t score) {
    m_score = score;
  }
};

}

#endif  // MERT_POINT_H
