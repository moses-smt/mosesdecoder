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
     const ScoreComponentCollection& gradient,
     ScoreComponentCollection* new_x) {
  assert(new_x);
  ++iteration_;
  cerr << "OPTIMIZER ITERATION #" << iteration_ << endl;
  cerr << "  GRADIENT: " << gradient << endl;
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

}

