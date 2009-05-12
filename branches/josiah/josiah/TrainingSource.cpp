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
  int wt_dump_freq,
  std::string wt_dump_stem)
    : rank(r),
      size(s),
      batch_size(bsize),
      iteration(),
      corpus(),
      keep_going(true),
      total_exp_gain(),
      scaling_gradient(),
      scaling_hessianV(),
      order(batch_size),
      rng(rseed),
      dist(0, sents->size() - 1),
      draw(rng, dist),
      randomize_batches(randomize),
      optimizer(o),
      total_ref_len(),
      total_exp_len(),
      quenching_temp(), 
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
  ScoreComponentCollection hessianV;
  IncorporateGradient(trans_len, ref_len, exp_gain, unreg_exp_gain, grad, decoder, hessianV);
} 
                                                
  
  
void ExpectedBleuTrainer::IncorporateGradient(
       const float trans_len,
       const float ref_len,
       const float exp_gain,
       const float unreg_exp_gain,
       const ScoreComponentCollection& grad,
       Decoder* decoder, 
       const ScoreComponentCollection& hessianV) {

  if (compute_scale_gradient) {
    size_t size = grad.data().size() -1;
    vector <float>  _grad(size); 
    vector <float>  _hessianV(size);
    
    copy(grad.data().begin(),grad.data().end()-1,_grad.begin());
    copy(hessianV.data().begin(),hessianV.data().end()-1,_hessianV.begin());
    ScoreComponentCollection thisGrad(_grad);
    ScoreComponentCollection thisHessianV(_hessianV);
    
    scaling_gradient += grad.data()[size];
    scaling_hessianV += hessianV.data()[size];
    gradient.PlusEquals(thisGrad);
    hessianV_.PlusEquals(thisHessianV);  
  }
  else {
    gradient.PlusEquals(grad);
    hessianV_.PlusEquals(hessianV);  
  }
  
  total_exp_gain += exp_gain;
  total_unreg_exp_gain += unreg_exp_gain;
  total_ref_len += ref_len;
  total_exp_len += trans_len;

  if (cur == cur_end) {
    vector<float> w;
    GetFeatureWeights(&w);
    
    vector<float> rcv_grad(w.size());
    vector<float> rcv_hessianV(w.size());
    assert(gradient.data().size() == w.size());
    assert(hessianV_.data().size() == w.size());
    
    if (compute_scale_gradient) {
      w.push_back(quenching_temp);
    }
    ScoreComponentCollection weights(w);
    
    float tg = 0, trl = 0, tel = 0, tgunreg = 0, tscalinggr= 0, tscalingHV = 0;
#ifdef MPI_ENABLED
    if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&gradient.data()[0]), &rcv_grad[0], w.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&hessianV_.data()[0]), &rcv_hessianV[0], w.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_exp_gain, &tg, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_unreg_exp_gain, &tgunreg, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_ref_len, &trl, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&total_exp_len, &tel, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&scaling_gradient, &tscalinggr, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    if (MPI_SUCCESS != MPI_Reduce(&scaling_hessianV, &tscalingHV, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
    
#else
    rcv_grad = gradient.data();
    rcv_hessianV = hessianV_.data();
    tg = total_exp_gain;
    tgunreg = total_exp_gain;
    trl = total_ref_len;
    tel = total_exp_len;
    tscalinggr = scaling_gradient;
    tscalingHV = scaling_hessianV;
#endif

    if (compute_scale_gradient) {
      rcv_grad.push_back(tscalinggr);
      rcv_hessianV.push_back(tscalingHV);
    }
     
    ScoreComponentCollection g(rcv_grad);
    ScoreComponentCollection hessV(rcv_hessianV);

    
    if (rank == 0) {
      tg /= batch_size;
      tgunreg /= batch_size;
      g.DivideEquals(batch_size);
      hessV.DivideEquals(batch_size);
      cerr << "TOTAL EXPECTED GAIN: " << tg << " (batch size = " << batch_size << ")\n";
      cerr << "TOTAL UNREGULARIZED EXPECTED GAIN: " << tgunreg << " (batch size = " << batch_size << ")\n";
      cerr << "EXPECTED LENGTH / REF LENGTH: " << tel << '/' << trl << " (" << (tel / trl) << ")\n";
      optimizer->Optimize(tg, weights, g, hessV, &weights);
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
    SetFeatureWeights(weights.data(), compute_scale_gradient);
    if (compute_scale_gradient) {
      quenching_temp = weights.data()[weights.data().size() - 1];
    }
    
    cur = cur_start;
    gradient.ZeroAll();
    hessianV_.ZeroAll();
    scaling_gradient = 0;
    scaling_hessianV = 0;
    total_exp_gain = 0;
    total_unreg_exp_gain = 0;
    total_exp_len = 0;
    total_ref_len = 0;
    
    if (rank == 0 && iteration > 0 && (iteration % weight_dump_freq) == 0) {
      stringstream s;
      s << weight_dump_stem;
      s << "_";
      s << iteration;
      string weight_file = s.str();
      cerr << "DUMPING WEIGHTS TO " << weight_file << endl;
      ofstream out(weight_file.c_str());
      if (out) {
        OutputWeights(out);
        out.close();
      }  else {
        cerr << "FAILED TO DUMP WEIGHTS" << endl;
      }
    }
    
  }
}

}

