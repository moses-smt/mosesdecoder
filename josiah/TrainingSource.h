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

struct ExpectedBleuTrainer : public InputSource {
  ExpectedBleuTrainer(
    int r,
    int s,
    int bsize,
    std::vector<std::string>* sents,
    unsigned int rseed,
    bool randomize,
    Optimizer* o);
  void ReserveNextBatch();
  virtual bool HasMore() const;
  virtual void GetSentence(std::string* sentence, int* lineno);
  void IncorporateGradient(
       const float trans_len,
       const float ref_len,
       const float exp_gain,
       const Moses::ScoreComponentCollection& grad,
       Decoder* decoder);

  int rank, size, batch_size;
  int iteration;
  int cur, cur_start;
  int cur_end;
  std::vector<std::string> corpus;
  bool keep_going;
  float total_exp_gain;
  Moses::ScoreComponentCollection gradient;
  std::vector<int> order;
  boost::mt19937 rng;
  boost::uniform_smallint<int> dist;
  boost::variate_generator<boost::mt19937, boost::uniform_smallint<int> > draw;
  bool randomize_batches;
  Optimizer* optimizer;
  float total_ref_len;
  float total_exp_len;
  int tlc;
};

}

