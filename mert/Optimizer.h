#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include <vector>
#include <string>
#include "FeatureData.h"
#include "Scorer.h"
#include "Types.h"

using namespace std;

typedef float featurescore;

class Point;

/**
 * Abstract optimizer class.
 */
class Optimizer
{
protected:
  Scorer *scorer;      // no accessor for them only child can use them
  FeatureData *FData;  // no accessor for them only child can use them
  unsigned int number_of_random_directions;

public:
  Optimizer(unsigned Pd, vector<unsigned> i2O, vector<parameter_t> start, unsigned int nrandom);
  void SetScorer(Scorer *_scorer);
  void SetFData(FeatureData *_FData);
  virtual ~Optimizer();

  unsigned size() const {
    return FData ? FData->size() : 0;
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
    return scorer->score(nbests);
  }

  statscore_t GetStatScore(const Point& param) const;

  vector<statscore_t> GetIncStatScore(vector<unsigned> ref, vector<vector<pair<unsigned,unsigned> > >) const;

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
  SimpleOptimizer(unsigned dim, vector<unsigned> i2O, vector<parameter_t> start, unsigned int nrandom)
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
  RandomDirectionOptimizer(unsigned dim, vector<unsigned> i2O, vector<parameter_t> start, unsigned int nrandom)
      : Optimizer(dim, i2O, start, nrandom), kEPS(0.0001) {}
  virtual statscore_t TrueRun(Point&) const;
};

/**
 * Dumb baseline optimizer: just picks a random point and quits.
 */
class RandomOptimizer : public Optimizer
{
public:
  RandomOptimizer(unsigned dim, vector<unsigned> i2O, vector<parameter_t> start, unsigned int nrandom)
      : Optimizer(dim, i2O, start, nrandom) {}
  virtual statscore_t TrueRun(Point&) const;
};

class OptimizerFactory
{
public:
  static vector<string> GetTypeNames();
  static Optimizer* BuildOptimizer(unsigned dim, vector<unsigned> tooptimize, vector<parameter_t> start, string type, unsigned int nrandom);

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

  static OptType GetOType(string);
  static vector<string> typenames;
  static void SetTypeNames();
};

#endif  // OPTIMIZER_H
