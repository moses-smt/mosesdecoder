#pragma once

#include <string>
#include <vector>

#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_smallint.hpp>


#include "FeatureVector.h"
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
       const Moses::FValue trans_len,
       const Moses::FValue ref_len,
       const Moses::FValue exp_gain,
       const Moses::FValue unreg_exp_gain,
       const Moses::FVector& grad);
  void IncorporateCorpusGradient(
      const Moses::FValue trans_len,
      const Moses::FValue ref_len,
      const Moses::FValue exp_gain,
      const Moses::FValue unreg_exp_gain,
      const Moses::FVector& grad);
  int GetCurr() { return cur;}
  int GetCurrEnd() { return cur_end;}

 private:
  int rank, size, batch_size;
  int cur, cur_start;
  int cur_end;
  std::vector<std::string> corpus;
  bool keep_going;
  Moses::FVector gradient;
  
  std::vector<int> order;
  boost::mt19937 rng;
  boost::uniform_smallint<int> dist;
  boost::variate_generator<boost::mt19937, boost::uniform_smallint<int> > draw;
  bool randomize_batches;
  Optimizer* optimizer;
  Moses::FValue total_ref_len;
  Moses::FValue total_exp_len;
  Moses::FValue total_exp_gain;
  Moses::FValue total_unreg_exp_gain;
  
  int tlc;
  int weight_dump_freq;
  std::string weight_dump_stem;
};

}

