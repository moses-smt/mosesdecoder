#include "OnlineLearnerTrainer.h"

#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <cassert>

#include "Optimizer.h"
#include "Decoder.h"
#include "OnlineLearner.h"

using namespace std;
using namespace Moses;

namespace Josiah {
  
    
  OnlineLearnerTrainer::OnlineLearnerTrainer(
                                             int r, 
                                             int s, 
                                             int bsize,
                                             vector<string>* sents,
                                             unsigned int rseed,
                                             bool randomize,
                                             int maxSents,
                                             int wt_dump_freq,
                                             std::string wt_dump_stem, 
                                             OnlineLearner* learner)
  : rank(r),
  size(s),
  batch_size(bsize),
  max_sents(maxSents),
  corpus(),
  keep_going(true),
  order(batch_size),
  rng(rseed),
  dist(0, sents->size() - 1),
  draw(rng, dist),
  randomize_batches(randomize),
  weight_dump_freq(wt_dump_freq),
  weight_dump_stem(wt_dump_stem), 
  batch_ctr(0),
  m_learner(learner){
    if (rank >= batch_size) keep_going = false;
    corpus.swap(*sents);
    int esize = min(batch_size, size);
    //cerr << "esize: " << esize << endl;
    int sents_per_batch = batch_size / esize;
    cur = cur_start = sents_per_batch * rank;
    //cerr << "sents_per_batch: " << sents_per_batch << endl;
    cur_end = min((int)corpus.size(), sents_per_batch * (rank + 1));
    if (rank == size - 1) cur_end = batch_size;
    cerr << rank << "/" << size << ": cur_start=" << cur_start << "  cur_end=" << cur_end << endl;
    assert(cur_end >= cur_start);
    tlc = 0;
    ReserveNextBatch();
  }
  
  void OnlineLearnerTrainer::ReserveNextBatch() {
    cur = cur_start;
    if (rank == 0) {
      if (randomize_batches) {
        for (unsigned int i = 0; i < order.size(); ++i, ++tlc)
          order[i] = draw();
      } else {
        for (unsigned int i = 0; i < order.size(); ++i, ++tlc)
          order[i] = tlc % corpus.size();
      }
    } 
#ifdef MPI_ENABLED
    if (MPI_SUCCESS != MPI_Bcast(&order[0], order.size(), MPI_INT, 0, MPI_COMM_WORLD))
      MPI_Abort(MPI_COMM_WORLD,1);
#endif
    if (weight_dump_freq > 0 && rank == 0 && batch_ctr > 0 && (batch_ctr % weight_dump_freq) == 0) {
      stringstream s;
      s << weight_dump_stem;
      s << "_";
      s << batch_ctr;
      string weight_file = s.str();
      cerr << "Dumping weights to  " << weight_file << endl;
      ofstream out(weight_file.c_str());
      ScoreComponentCollection avgWeights = m_learner->GetAveragedWeights();
      
      if (out) {
        OutputWeights(avgWeights.data(), out);
        out.close();
      }  else {
        cerr << "Failed to dump weights" << endl;
      }
    }
   
    if (rank == 0) {
      batch_ctr++;
    } 
  }
  
  bool OnlineLearnerTrainer::HasMore() const {
    return keep_going && (tlc < max_sents) && (cur < cur_end);
  }
  
  void OnlineLearnerTrainer::GetSentence(string* sentence, int* lineno) {
    assert(static_cast<unsigned int>(cur) < order.size());
    if (lineno) *lineno = order[cur];
    *sentence = corpus[order[cur++]];
  }
  
}

