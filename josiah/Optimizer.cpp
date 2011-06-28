#include "Optimizer.h"

#include <iostream>


using namespace Moses;
using namespace std;

namespace Josiah {

Optimizer::~Optimizer() {}

  

void Optimizer::Optimize(
     FValue f,
     const FVector x,
     const FVector& gr,
     FVector* new_x
     ) {
  ++iteration_;
  FVector gradient = gr;
  if (use_gaussian_prior_) {
    gradient -= mean_;
    gradient /= variance_;
  }
  cerr << "OPTIMIZER ITERATION #" << iteration_ << endl;
  cerr << "  CURR VALUES: " << x << endl;
  cerr << "  GRADIENT: " << gr << endl;
  if (use_gaussian_prior_)
    cerr << "P-GRADIENT: " << gradient << endl;
  OptimizeImpl(f, x, gradient,  new_x);
  cerr << "NEW VALUES: " << *new_x << endl;
  if (HasConverged()) {
    cerr << "OPTIMIZER CONVERGED IN " << iteration_ << " ITERATIONS.\n";
  } else if (GetIteration() >= max_iterations_) {
    cerr << "OPTIMIZER REACHED MAX ITERATIONS. STOPPING.\n";
    SetHasConverged();
  }
}

void DumbStochasticGradientDescent::OptimizeImpl(
     FValue f,
     const FVector& x,
     const FVector& gradient,
     FVector* new_x) {
  FVector g = gradient;
  g *= eta_;
  *new_x = x;
  *new_x += g;
}

void ExponentiatedGradientDescent::OptimizeImpl(
     FValue,
     const FVector& x,
     const FVector& gradient,
     FVector* new_x) {
  //for (unsigned i = 0; i < eta_.size(); ++i) {
    //eta_[i] = eta_[i] * max(min_multiplier_, 1.0f + mu_ * gradient[i] * (eta_[i] * prev_g_[i]));
    eta_ *= fvmax(min_multiplier_, 1.0 + mu_ * gradient * eta_ * prev_g_);
  //}
  cerr << "ETA: " << eta_ << endl;
  *new_x = gradient;
  *new_x *= eta_;
  *new_x += x;
  cerr << "New x: " << *new_x << endl;
  prev_g_ = gradient;
}
  
void MetaNormalizedExponentiatedGradientDescent::OptimizeImpl(
     FValue,
     const FVector& x,
     const FVector& gradient,
     FVector* new_x) {
  

    
  cerr << "Curr x: " << x << endl;
  //for (unsigned i = 0; i < v_.size(); ++i) {
    //v_[i] = gamma_ * v_[i] + ((1 - gamma_) * gradient[i] * gradient[i]);
    v_ = gamma_ * v_ + ((1-gamma_) * gradient * gradient);
  //}
    
  //for (unsigned i = 0; i < eta_.size(); ++i) {
    //eta_[i] = eta_[i] * max(min_multiplier_, 1.0f + ((mu_ * gradient[i] *  prev_g_[i])/ v_[i]));
    eta_ = eta_ * fvmax(min_multiplier_, 1 + ((mu_ * gradient * prev_g_) / v_));
  //}
  
  cerr << "ETA: " << eta_ << endl;
  *new_x = gradient;
  *new_x *= eta_;
  cerr << "Gradient * ETA: " << *new_x << endl;
  *new_x += x;
  cerr << "New x: " << *new_x << endl;
  prev_g_ = gradient;
}
  

  
}

