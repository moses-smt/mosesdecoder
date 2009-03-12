#include "TrainingSource.h"

#ifdef MPI_ENABLED
#include <mpi.h>
#endif

#include <cassert>

#include "Optimizer.h"
#include "Decoder.h"

using namespace std;
using namespace Moses;

namespace Josiah {

ExpectedBleuTrainer::ExpectedBleuTrainer(
  int r, 
  int s, 
  int bsize,
  vector<string>* sents,
  unsigned int rseed,
  bool randomize,
  Optimizer* o)
    : rank(r),
      size(s),
      batch_size(bsize),
      iteration(),
      corpus(),
      keep_going(true),
      total_exp_gain(),
      order(batch_size),
      rng(rseed),
      dist(0, sents->size() - 1),
      draw(rng, dist),
      randomize_batches(randomize),
      optimizer(o),
      total_ref_len(),
      total_exp_len() {
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

void ExpectedBleuTrainer::ReserveNextBatch() {
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
  }

bool ExpectedBleuTrainer::HasMore() const {
  return keep_going && (cur < cur_end);
}

void ExpectedBleuTrainer::GetSentence(string* sentence, int* lineno) {
  assert(static_cast<unsigned int>(cur) < order.size());
  if (lineno) *lineno = order[cur];
  *sentence = corpus[order[cur++]];
}

void ExpectedBleuTrainer::IncorporateGradient(
       const float trans_len,
       const float ref_len,
       const float exp_gain,
       const ScoreComponentCollection& grad,
       Decoder* decoder) {
  gradient.PlusEquals(grad);
  total_exp_gain += exp_gain;
  total_ref_len += ref_len;
  total_exp_len += trans_len;

  if (cur == cur_end) {
    vector<float> w;
    GetFeatureWeights(&w);
    ScoreComponentCollection weights(w);
    vector<float> rcv_grad(w.size());
    assert(gradient.data().size() == w.size());
    float tg = 0, trl = 0, tel = 0;
#ifdef MPI_ENABLED
    if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&gradient.data()[0]), &rcv_grad[0], w.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_exp_gain, &tg, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_ref_len, &trl, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_exp_len, &tel, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
#else
    rcv_grad = gradient.data();
    tg = total_exp_gain;
    trl = total_ref_len;
    tel = total_exp_len;
#endif
    ScoreComponentCollection g(rcv_grad);
    if (rank == 0) {
      tg /= batch_size;
      g.DivideEquals(batch_size);
      cerr << "TOTAL EXPECTED GAIN: " << tg << " (batch size = " << batch_size << ")\n";
      cerr << "EXPECTED LENGTH / REF LENGTH: " << tel << '/' << trl << " (" << (tel / trl) << ")\n";
      optimizer->Optimize(tg, weights, g, &weights);
      weights.PlusEquals(g);
      if (optimizer->HasConverged()) keep_going = false;
    }
#ifdef MPI_ENABLED
    int kg = keep_going;
    if (MPI_SUCCESS != MPI_Bcast(const_cast<float*>(&weights.data()[0]), weights.data().size(), MPI_FLOAT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Bcast(&kg, 1, MPI_INT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    ReserveNextBatch();
    if (MPI_SUCCESS != MPI_Barrier(MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    keep_going = kg;
#endif
    SetFeatureWeights(weights.data());
    cur = cur_start;
    gradient.ZeroAll();
    total_exp_gain = 0;
    total_exp_len = 0;
    total_ref_len = 0;
  }
}

}

