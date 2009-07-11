#pragma once

#include <string>
#include <vector>

#include <boost/random/variate_generator.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_smallint.hpp>

#include "ScoreComponentCollection.h"
#include "InputSource.h"
#include "OnlineLearner.h"

namespace Josiah {
  
class Decoder;
  
class OnlineLearnerTrainer : public InputSource {
  public:
    OnlineLearnerTrainer(
                         int r,      // MPI rank, or 0 if not MPI
                         int s,      // MPI size, or 1 if not MPI
                         int bsize,  // batch size
                         std::vector<std::string>* sents,  // development corpus
                         unsigned int rseed,
                         bool randomize,
                         int maxSents,
                         int wt_dump_freq,
                         std::string wt_dump_stem, 
                         OnlineLearner* learner);
    void ReserveNextBatch();
    virtual bool HasMore() const;
    virtual void GetSentence(std::string* sentence, int* lineno);
    int GetCurr() { return cur;}
    int GetCurrEnd() { return cur_end;}
    
  private:
    int rank, size, batch_size;
    int max_sents;
    int cur, cur_start;
    int cur_end;
    std::vector<std::string> corpus;
    int tlc;
    bool keep_going;
    std::vector<int> order;
    boost::mt19937 rng;
    boost::uniform_smallint<int> dist;
    boost::variate_generator<boost::mt19937, boost::uniform_smallint<int> > draw;
    bool randomize_batches;
    int weight_dump_freq;
    std::string weight_dump_stem;
    int batch_ctr;
    OnlineLearner* m_learner;
  };
  
}

