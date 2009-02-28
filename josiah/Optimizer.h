#pragma once

#include <vector>
#include "ScoreComponentCollection.h"

namespace Josiah {

struct Optimizer {
  Optimizer(int max_iterations) 
    : iteration_(0),
      converged_(false),
      max_iterations_(max_iterations),
      use_gaussian_prior_(false) {}
  virtual ~Optimizer();

  void SetUseGaussianPrior(const std::vector<float>& means,
                           const float variance) {
    use_gaussian_prior_ = true;
    means_ = means;
    variance_ = variance;
  }

  void Optimize(
     float f, // if known
     const Moses::ScoreComponentCollection x,  // not ref! don't change!
     const Moses::ScoreComponentCollection& gradient,
     Moses::ScoreComponentCollection* new_x);

  bool HasConverged() const {
    return converged_;
  }

  int GetIteration() const {
    return iteration_;
  }

 protected:
  virtual void OptimizeImpl(
     float f, // if known
     const Moses::ScoreComponentCollection& x,
     const Moses::ScoreComponentCollection& gradient,
     Moses::ScoreComponentCollection* new_x) = 0;

  void SetHasConverged(bool converged = true) {
    converged_ = converged;
  }

 private:
  int iteration_;
  bool converged_;
  int max_iterations_;
  bool use_gaussian_prior_;
  std::vector<float> means_;      // for gaussian prior
  float variance_;                // for gaussian prior
};

class DumbStochasticGradientDescent : public Optimizer {
 public:
  DumbStochasticGradientDescent(float eta, int max_iters) :
    Optimizer(max_iters), eta_(eta) {}

  virtual void OptimizeImpl(
     float f,
     const Moses::ScoreComponentCollection& x,
     const Moses::ScoreComponentCollection& gradient,
     Moses::ScoreComponentCollection* new_x);

 private:
  float eta_;
};

// see N. Schraudolph (1999) Local Gain Adaptation in Stochastic Gradient
// Descent, Technical Report IDSIA-09-99, p. 2.
// No, this isn't stochastic metadescent, but EGD is described there too
class ExponentiatedGradientDescent : public Optimizer {
 public:
  ExponentiatedGradientDescent(const Moses::ScoreComponentCollection& eta,
      float mu, float min_multiplier, int max_iters) :
    Optimizer(max_iters), eta_(eta), mu_(mu), min_multiplier_(min_multiplier), prev_g_(eta) {
    prev_g_.ZeroAll();
  }

  virtual void OptimizeImpl(
     float f,
     const Moses::ScoreComponentCollection& x,
     const Moses::ScoreComponentCollection& gradient,
     Moses::ScoreComponentCollection* new_x);

 private:
  Moses::ScoreComponentCollection eta_;
  const float mu_;
  const float min_multiplier_;
  Moses::ScoreComponentCollection prev_g_;
};

}

