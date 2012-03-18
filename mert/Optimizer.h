#ifndef MERT_OPTIMIZER_H_
#define MERT_OPTIMIZER_H_

#include <vector>
#include <string>
#include "Data.h"
#include "FeatureData.h"
#include "Scorer.h"
#include "Types.h"

using namespace std;

class Point;

/**
 * Abstract optimizer class.
 */
class Optimizer
{
protected:
  Scorer *m_scorer;      // no accessor for them only child can use them
  FeatureDataHandle m_feature_data;  // no accessor for them only child can use them
  unsigned int m_num_random_directions;

public:
  Optimizer(unsigned Pd, const vector<unsigned>& i2O, const vector<parameter_t>& start, unsigned int nrandom);

  void SetScorer(Scorer *scorer) { m_scorer = scorer; }
  void SetFeatureData(FeatureDataHandle feature_data) { m_feature_data = feature_data; }
  virtual ~Optimizer();

  unsigned size() const {
    return m_feature_data ? m_feature_data->size() : 0;
  }

  /**
   * Generic wrapper around TrueRun to check a few things. Non virtual.
   */
  statscore_t Run(Point&) const;

  /**
   * Main function that performs an optimization.
   */
  virtual statscore_t TrueRun(Point&) const = 0;

  /**
   * Given a set of lambdas, get the nbest for each sentence.
   */
  void Get1bests(const Point& param,vector<unsigned>& bests) const;

  /**
   * Given a set of nbests, get the Statistical score.
   */
  statscore_t GetStatScore(const vector<unsigned>& nbests) const {
    return m_scorer->score(nbests);
  }

  statscore_t GetStatScore(const Point& param) const;

  vector<statscore_t> GetIncStatScore(const vector<unsigned>& ref, const vector<vector<pair<unsigned,unsigned> > >& diffs) const;

  /**
   * Get the optimal Lambda and the best score in a particular direction from a given Point.
   */
  statscore_t LineOptimize(const Point& start, const Point& direction, Point& best) const;
};


/**
 * Default basic optimizer.
 * This class implements Powell's method.
 */
class SimpleOptimizer : public Optimizer
{
private:
  const float kEPS;
public:
  SimpleOptimizer(unsigned dim, const vector<unsigned>& i2O, const vector<parameter_t>& start, unsigned int nrandom)
      : Optimizer(dim, i2O, start,nrandom), kEPS(0.0001) {}
  virtual statscore_t TrueRun(Point&) const;
};

/**
 * An optimizer with random directions.
 */
class RandomDirectionOptimizer : public Optimizer
{
private:
  const float kEPS;
public:
  RandomDirectionOptimizer(unsigned dim, const vector<unsigned>& i2O, const vector<parameter_t>& start, unsigned int nrandom)
      : Optimizer(dim, i2O, start, nrandom), kEPS(0.0001) {}
  virtual statscore_t TrueRun(Point&) const;
};

/**
 * Dumb baseline optimizer: just picks a random point and quits.
 */
class RandomOptimizer : public Optimizer
{
public:
  RandomOptimizer(unsigned dim, const vector<unsigned>& i2O, const vector<parameter_t>& start, unsigned int nrandom)
      : Optimizer(dim, i2O, start, nrandom) {}
  virtual statscore_t TrueRun(Point&) const;
};

class OptimizerFactory
{
public:
  static vector<string> GetTypeNames();
  static Optimizer* BuildOptimizer(unsigned dim, const vector<unsigned>& to_optimize, const vector<parameter_t>& start, const string& type, unsigned int nrandom);

private:
  OptimizerFactory() {}
  ~OptimizerFactory() {}

  // Add new optimizer here BEFORE NOPTIMZER
  enum OptType {
    POWELL = 0,
    RANDOM_DIRECTION = 1,
    RANDOM,
    NOPTIMIZER
  };

  // Get optimizer type.
  static OptType GetOType(const string& type);

  // Setup optimization types.
  static void SetTypeNames();

  static vector<string> m_type_names;
};

#endif  // OPTIMIZER_H
