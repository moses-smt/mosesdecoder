#pragma once

namespace Moses { class ScoreComponentCollection; }

namespace Josiah {

struct Optimizer {
  Optimizer(int max_iterations) 
    : iteration_(0), converged_(false), max_iterations_(max_iterations) {}
  virtual ~Optimizer();

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
};

class DumbStochasticGradientDescent : public Optimizer {
 public:
  DumbStochasticGradientDescent() : Optimizer(8), eta_(0.75f) {}
  DumbStochasticGradientDescent(float eta, int max_iters) :
    Optimizer(max_iters), eta_(eta) {}

  virtual void OptimizeImpl(
     float f,
     const Moses::ScoreComponentCollection& x,
     const Moses::ScoreComponentCollection& gradient,
     Moses::ScoreComponentCollection* new_x);

 private:
  float eta_;
  int max_iterations_;
};

}
