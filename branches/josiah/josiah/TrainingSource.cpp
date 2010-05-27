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
  Optimizer* o,
  int init_iteration_number,
  int wt_dump_freq,
  std::string wt_dump_stem)
    : rank(r),
      size(s),
      batch_size(bsize),
      iteration(init_iteration_number),
      corpus(),
      keep_going(true),
      order(batch_size),
      rng(rseed),
      dist(0, sents->size() - 1),
      draw(rng, dist),
      randomize_batches(randomize),
      optimizer(o),
      total_ref_len(),
      total_exp_len(),
      total_exp_gain(),
      total_unreg_exp_gain(),
      weight_dump_freq(wt_dump_freq),
      weight_dump_stem(wt_dump_stem){
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
       const float unreg_exp_gain,
       const ScoreComponentCollection& grad,
       Decoder* decoder) {

  gradient.PlusEquals(grad);
  total_exp_gain += exp_gain;
  total_unreg_exp_gain += unreg_exp_gain;
  total_ref_len += ref_len;
  total_exp_len += trans_len;
  
  if (cur == cur_end) {
    vector<float> w;
    GetFeatureWeights(&w);
    
    vector<float> rcv_grad(w.size());
    assert(gradient.data().size() == w.size());
    
    
    float tg = 0, trl = 0, tel = 0, tgunreg = 0;
#ifdef MPI_ENABLED
    if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&gradient.data()[0]), &rcv_grad[0], w.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_exp_gain, &tg, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_unreg_exp_gain, &tgunreg, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_ref_len, &trl, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_exp_len, &tel, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
#else
    rcv_grad = gradient.data();
    tg = total_exp_gain;
    tgunreg = total_exp_gain;
    trl = total_ref_len;
    tel = total_exp_len;
#endif

    ScoreComponentCollection weights(w);
    ScoreComponentCollection g(rcv_grad);
    
    if (rank == 0) {
      tg /= batch_size;
      tgunreg /= batch_size;
      g.DivideEquals(batch_size);
      cerr << "TOTAL EXPECTED GAIN: " << tg << " (batch size = " << batch_size << ")\n";
      cerr << "TOTAL UNREGULARIZED EXPECTED GAIN: " << tgunreg << " (batch size = " << batch_size << ")\n";
      cerr << "EXPECTED LENGTH / REF LENGTH: " << tel << '/' << trl << " (" << (tel / trl) << ")\n";
      optimizer->Optimize(tg, weights, g,  &weights);
      if (optimizer->HasConverged()) keep_going = false;
    }
#ifdef MPI_ENABLED
    int kg = keep_going;
    int iteration = optimizer->GetIteration();
    
    if (MPI_SUCCESS != MPI_Bcast(const_cast<float*>(&weights.data()[0]), weights.data().size(), MPI_FLOAT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Bcast(&kg, 1, MPI_INT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Bcast(&iteration, 1, MPI_INT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    ReserveNextBatch();
    if (MPI_SUCCESS != MPI_Barrier(MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    keep_going = kg;
    optimizer->SetIteration(iteration);
#endif
    SetFeatureWeights(weights.data());
    
    cur = cur_start;
    gradient.ZeroAll();
    total_exp_gain = 0;
    total_unreg_exp_gain = 0;
    total_exp_len = 0;
    total_ref_len = 0;
    
    if (weight_dump_freq > 0 && rank == 0 && iteration > 0 && (iteration % weight_dump_freq) == 0) {
      stringstream s;
      s << weight_dump_stem;
      s << "_";
      s << iteration;
      string weight_file = s.str();
      cerr << "Dumping weights to  " << weight_file << endl;
      ofstream out(weight_file.c_str());
      if (out) {
        OutputWeights(out);
        out.close();
      }  else {
        cerr << "Failed to dump weights" << endl;
      }
    }
    
  }
}

void ExpectedBleuTrainer::IncorporateCorpusGradient(
                                                const float trans_len,
                                                const float ref_len,      
                                                const float exp_gain,
                                                const float unreg_exp_gain,
                                                const ScoreComponentCollection& grad,
                                                Decoder* decoder) {
    
    if (cur == cur_end) {
      vector<float> w;
      GetFeatureWeights(&w);
   
      ScoreComponentCollection weights(w);
      
      if (rank == 0) {
        cerr << "TOTAL EXPECTED GAIN: " << exp_gain << " (batch size = " << batch_size << ")\n";
        cerr << "TOTAL UNREGULARIZED EXPECTED GAIN: " << unreg_exp_gain << " (batch size = " << batch_size << ")\n";
        cerr << "EXPECTED LENGTH / REF LENGTH: " << trans_len << '/' << ref_len << " (" << (trans_len / ref_len) << ")\n";
        optimizer->Optimize(exp_gain, weights, grad,  &weights);
        if (optimizer->HasConverged()) keep_going = false;
      }
#ifdef MPI_ENABLED
      int kg = keep_going;
      int iteration = optimizer->GetIteration();
      
      if (MPI_SUCCESS != MPI_Bcast(const_cast<float*>(&weights.data()[0]), weights.data().size(), MPI_FLOAT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      if (MPI_SUCCESS != MPI_Bcast(&kg, 1, MPI_INT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      if (MPI_SUCCESS != MPI_Bcast(&iteration, 1, MPI_INT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      ReserveNextBatch();
      if (MPI_SUCCESS != MPI_Barrier(MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
      keep_going = kg;
      optimizer->SetIteration(iteration);
#endif
      SetFeatureWeights(weights.data());
      
      
      cur = cur_start;
      
      if (weight_dump_freq > 0 && rank == 0 && iteration > 0 && (iteration % weight_dump_freq) == 0) {
        stringstream s;
        s << weight_dump_stem;
        s << "_";
        s << iteration;
        string weight_file = s.str();
        cerr << "Dumping weights to  " << weight_file << endl;
        ofstream out(weight_file.c_str());
        if (out) {
          OutputWeights(out);
          out.close();
        }  else {
          cerr << "Failed to dump weights" << endl;
        }
      }
      
    }
  }  
  
}

