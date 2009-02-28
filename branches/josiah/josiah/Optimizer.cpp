#include "Optimizer.h"

#include <iostream>

#include "ScoreComponentCollection.h"

using namespace std;
using namespace Moses;

namespace Josiah {

Optimizer::~Optimizer() {}

void Optimizer::Optimize(
     float f,
     const ScoreComponentCollection x,
     const ScoreComponentCollection& gr,
     ScoreComponentCollection* new_x) {
  assert(new_x);
  assert(x.size() == gr.size());
  assert(x.size() == new_x->size());
  ++iteration_;
  ScoreComponentCollection gradient = gr;
  if (use_gaussian_prior_) {
    assert(means_.size() == gradient.size());
    for (size_t i = 0; i < gradient.size(); ++i)
      gradient[i] -= (x[i] - means_[i]) / variance_;
  }
  cerr << "OPTIMIZER ITERATION #" << iteration_ << endl;
  cerr << "  GRADIENT: " << gr << endl;
  if (use_gaussian_prior_)
    cerr << "P-GRADIENT: " << gradient << endl;
  OptimizeImpl(f, x, gradient, new_x);
  cerr << "NEW VALUES: " << *new_x << endl;
  if (HasConverged()) {
    cerr << "OPTIMIZER CONVERGED IN " << iteration_ << " ITERATIONS.\n";
  } else if (GetIteration() >= max_iterations_) {
    cerr << "OPTIMIZER REACHED MAX ITERATIONS. STOPPING.\n";
    SetHasConverged();
  }
}

void DumbStochasticGradientDescent::OptimizeImpl(
     float f,
     const ScoreComponentCollection& x,
     const ScoreComponentCollection& gradient,
     ScoreComponentCollection* new_x) {
  (void) f;  // don't care about the function value!
  ScoreComponentCollection g = gradient;
  g.MultiplyEquals(eta_);
  *new_x = x;
  new_x->PlusEquals(g);
}

void ExponentiatedGradientDescent::OptimizeImpl(
     float f,
     const ScoreComponentCollection& x,
     const ScoreComponentCollection& gradient,
     ScoreComponentCollection* new_x) {
  (void) f;
  assert(x.size() == eta_.size());
  for (unsigned i = 0; i < eta_.size(); ++i) {
    eta_[i] = eta_[i] * max(min_multiplier_, 1.0f + mu_ * gradient[i] * (eta_[i] * prev_g_[i]));
  }
  cerr << "ETA: " << eta_ << endl;
  *new_x = gradient;
  new_x->MultiplyEquals(eta_);
  new_x->PlusEquals(x);
  prev_g_ = gradient;
}

}

