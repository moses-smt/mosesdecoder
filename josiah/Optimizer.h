#pragma once

#include <vector>

#include "FeatureVector.h"

namespace Josiah {

struct Optimizer {
  Optimizer(int max_iterations) 
    : iteration_(0),
      converged_(false),
      max_iterations_(max_iterations),
      use_gaussian_prior_(false) {}
  virtual ~Optimizer();

  void SetUseGaussianPrior(const Moses::FValue mean,
                           const Moses::FValue variance) {
    use_gaussian_prior_ = true;
    mean_ = mean;
    variance_ = variance;
  }

  void Optimize(
     Moses::FValue f, // if known
     const Moses::FVector x,  // not ref! don't change!
     const Moses::FVector& gradient,
     Moses::FVector* new_x);

  bool HasConverged() const {
    return converged_;
  }

  int GetIteration() const {
    return iteration_;
  }

  void SetIteration(int iteration) { 
    iteration_ = iteration;
  }
  
 protected:
  
  virtual void OptimizeImpl(
     float f, // if known
     const Moses::FVector& x,
     const Moses::FVector& gradient,
     Moses::FVector* new_x) = 0;
  
  
  void SetHasConverged(bool converged = true) {
    converged_ = converged;
  }

 private:
  int iteration_;
  bool converged_;
  int max_iterations_;
  bool use_gaussian_prior_;
  Moses::FValue mean_;      // for gaussian prior
  Moses::FValue variance_;                // for gaussian prior
  
};

class DumbStochasticGradientDescent : public Optimizer {
 public:
  DumbStochasticGradientDescent(Moses::FValue eta, int max_iters) :
    Optimizer(max_iters), eta_(eta) {}

  virtual void OptimizeImpl(
     float f,
     const Moses::FVector& x,
     const Moses::FVector& gradient,
     Moses::FVector* new_x);

 private:
  Moses::FValue eta_;
};

// see N. Schraudolph (1999) Local Gain Adaptation in Stochastic Gradient
// Descent, Technical Report IDSIA-09-99, p. 2.
// No, this isn't stochastic metadescent, but EGD is described there too
class ExponentiatedGradientDescent : public Optimizer {
 public:
  ExponentiatedGradientDescent(const Moses::FVector& eta,
      Moses::FValue mu, Moses::FValue min_multiplier, int max_iters, const Moses::FVector& prev_gradient) :
    Optimizer(max_iters), eta_(eta), mu_(mu), min_multiplier_(min_multiplier), prev_g_(prev_gradient) { 
     //std::cerr << "Eta : " << eta_ << std::endl;
     //std::cerr << "Prev gradient : " << prev_g_ << std::endl;
   }

  void SetPreviousGradient(const Moses::FVector& prev_g) { prev_g_ = prev_g;}
  void SetEta(const Moses::FVector& eta) { eta_ = eta;}
  
  
  
  virtual void OptimizeImpl(
                            Moses::FValue,
                            const Moses::FVector& x,
                            const Moses::FVector& gradient,
                            Moses::FVector* new_x);
  
 
  
 protected:
  Moses::FVector eta_;
  const Moses::FValue mu_;
  const Moses::FValue min_multiplier_;
  Moses::FVector prev_g_;
};

class MetaNormalizedExponentiatedGradientDescent : public ExponentiatedGradientDescent {
 public:
  MetaNormalizedExponentiatedGradientDescent(const Moses::FVector& eta,
                                 Moses::FValue mu, Moses::FValue min_multiplier, Moses::FValue gamma, int max_iters, const Moses::FVector& prev_gradient) :
  ExponentiatedGradientDescent(eta, mu, min_multiplier, max_iters, prev_gradient), v_(eta), gamma_(gamma) {
    std::cerr << " MetaNormalizedExponentiatedGradientDescent, gamma : " << gamma << std::endl;
  }
    
    
  virtual void OptimizeImpl(
                            Moses::FValue f,
                            const Moses::FVector& x,
                            const Moses::FVector& gradient,
                            Moses::FVector* new_x);
  
    
  private:
    Moses::FVector v_;
    Moses::FValue gamma_;
  };
  
  
}

