#pragma once

#include <string>
#include <vector>

#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_smallint.hpp>

#include "ScoreComponentCollection.h"
#include "InputSource.h"

namespace Josiah {
 
class Decoder;
class Optimizer;

class ExpectedBleuTrainer : public InputSource {
 public:
  ExpectedBleuTrainer(
    int r,      // MPI rank, or 0 if not MPI
    int s,      // MPI size, or 1 if not MPI
    int bsize,  // batch size
    std::vector<std::string>* sents,  // development corpus
    unsigned int rseed,
    bool randomize,
    Optimizer* o,
    int wt_dump_freq,
    std::string wt_dump_stem);
  void ReserveNextBatch();
  virtual bool HasMore() const;
  virtual void GetSentence(std::string* sentence, int* lineno);
  void IncorporateGradient(
       const float trans_len,
       const float ref_len,
       const float exp_gain,
       const float unreg_exp_gain,
       const Moses::ScoreComponentCollection& grad,
       Decoder* decoder, const float scaling_gradient);
  void IncorporateGradient(
                           const float trans_len,
                           const float ref_len,
                           const float exp_gain,
                           const float unreg_exp_gain,
                           const Moses::ScoreComponentCollection& grad,
                           Decoder* decoder, 
                           const Moses::ScoreComponentCollection& hessianV,
                           const float scaling_gradient);
  void SetComputeScaleGradient(bool scale_gradient) {compute_scale_gradient = scale_gradient;}
  float GetCurrQuenchingTemp() {return quenching_temp;}

 private:
  int rank, size, batch_size;
  int iteration;
  int cur, cur_start;
  int cur_end;
  std::vector<std::string> corpus;
  bool keep_going;
  Moses::ScoreComponentCollection gradient;
  Moses::ScoreComponentCollection hessianV_;
  
  std::vector<int> order;
  boost::mt19937 rng;
  boost::uniform_smallint<int> dist;
  boost::variate_generator<boost::mt19937, boost::uniform_smallint<int> > draw;
  bool randomize_batches;
  Optimizer* optimizer;
  float total_ref_len;
  float total_exp_len;
  float total_scaling_gradient;
  float total_scaling_hessianV;
  float total_exp_gain;
  float total_unreg_exp_gain;
  
  int tlc;
  bool compute_scale_gradient;
  float quenching_temp;
  int weight_dump_freq;
  std::string weight_dump_stem;
};

}

