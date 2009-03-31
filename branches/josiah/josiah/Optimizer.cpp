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
 
  ScoreComponentCollection hessianV = gr;
  hessianV.ZeroAll();
  Optimize(f,x,gr,hessianV, new_x);
}  
void Optimizer::Optimize(
     float f,
     const ScoreComponentCollection x,
     const ScoreComponentCollection& gr,
     const ScoreComponentCollection& hessianV,                    
     ScoreComponentCollection* new_x
     ) {
  assert(new_x);
  assert(x.size() == gr.size());
  assert(x.size() == hessianV.size()); 
  assert(x.size() == new_x->size());
  ++iteration_;
  ScoreComponentCollection gradient = gr;
  if (use_gaussian_prior_) {
    assert(means_.size() == gradient.size());
    for (size_t i = 0; i < gradient.size(); ++i)
      gradient[i] -= (x[i] - means_[i]) / variance_;
  }
  cerr << "OPTIMIZER ITERATION #" << iteration_ << endl;
  cerr << "  CURR VALUES: " << x << endl;
  cerr << "  GRADIENT: " << gr << endl;
  if (use_gaussian_prior_)
    cerr << "P-GRADIENT: " << gradient << endl;
  OptimizeImpl(f, x, gradient, hessianV, new_x);
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
     const ScoreComponentCollection& hessianV, 
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
     const ScoreComponentCollection& hessianV,                                           
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
  cerr << "New x: " << *new_x << endl;
  prev_g_ = gradient;
}

void MetaNormalizedExponentiatedGradientDescent::OptimizeImpl(
     float f,
     const ScoreComponentCollection& x,
     const ScoreComponentCollection& gradient,
     const ScoreComponentCollection& hessianV,              
     ScoreComponentCollection* new_x) {
  
  (void) f;
  assert(x.size() == eta_.size());
  assert(x.size() == v_.size());
    
  cerr << "Curr x: " << x << endl;
  for (unsigned i = 0; i < v_.size(); ++i) {
    v_[i] = gamma_ * v_[i] + ((1 - gamma_) * gradient[i] * gradient[i]);  
  }
    
  for (unsigned i = 0; i < eta_.size(); ++i) {
    eta_[i] = eta_[i] * max(min_multiplier_, 1.0f + ((mu_ * gradient[i] *  prev_g_[i])/ v_[i]));
  }
  
  cerr << "ETA: " << eta_ << endl;
  *new_x = gradient;
  new_x->MultiplyEquals(eta_);
  cerr << "Gradient * ETA: " << *new_x << endl;
  new_x->PlusEquals(x);
  cerr << "New x: " << *new_x << endl;
  prev_g_ = gradient;
}
  
void StochasticMetaDescent::OptimizeImpl(
     float f,
     const ScoreComponentCollection& x,
     const ScoreComponentCollection& gradient,
     const ScoreComponentCollection& hessianV,
     ScoreComponentCollection* new_x) {
  (void) f;
  assert(x.size() == eta_.size());
  assert(x.size() == v_.size());
    
  cerr << "Curr x: " << x << endl;
    
  for (unsigned i = 0; i < v_.size(); ++i) {
    v_[i] = lambda_ * v_[i]  - (eta_[i] * (gradient[i] + lambda_ * hessianV[i]));  
  }
    
  for (unsigned i = 0; i < eta_.size(); ++i) {
    eta_[i] = eta_[i] * max(min_multiplier_, 1.0f - (mu_ * gradient[i] *  v_[i]));
  }
    
  cerr << "ETA: " << eta_ << endl;
  *new_x = gradient;
  new_x->MultiplyEquals(eta_);
  new_x->MultiplyEquals(-1.0f);
  cerr << "-Gradient * ETA: " << *new_x << endl;
  new_x->PlusEquals(x);
  cerr << "New x: " << *new_x << endl;
    
  prev_g_ = gradient;
}
  
}

