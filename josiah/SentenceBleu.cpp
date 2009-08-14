#include "SentenceBleu.h"

#include <valarray>
#include <vector>

#include "Phrase.h"
#include "GibblerExpectedLossTraining.h"

using namespace std;

namespace Josiah {


BleuDefaultSmoothingSufficientStats SentenceBLEU::m_currentSmoothing;   
bool SentenceBLEU::computeApproxDocBLEU = false;
  
#define  BP_DENUM_HACK 100
SentenceBLEU::SentenceBLEU(int n, const std::vector<std::string>& refs, const std::string & src, float bp_scale, bool denum_hack) :
 n_(n), _bp_scale(bp_scale),  _use_bp_denum_hack(denum_hack) {
  for (vector<string>::const_iterator ci = refs.begin();
     ci != refs.end(); ++ci) {
    vector<const Factor*> fv;
    GainFunction::ConvertStringToFactorArray(*ci, &fv);
    lengths_.push_back(fv.size());
    CountRef(fv, ngrams_);
  }
  vector<const Factor*> fv;
  GainFunction::ConvertStringToFactorArray(src, &fv);
  m_src_len = fv.size(); 
}

SentenceBLEU::SentenceBLEU(int n, const vector<const Factor*> & ref, int src_len, float bp_scale, bool denum_hack) :
  n_(n) , m_src_len(src_len), _bp_scale(bp_scale),  _use_bp_denum_hack(denum_hack) {
  lengths_.push_back(ref.size());
  CountRef(ref, ngrams_);
    
}  
  
  
void SentenceBLEU::CountRef(const vector<const Factor*>& ref, NGramCountMap& ngrams) const{
  NGramCountMap tc;

  vector<const Factor*> ngram(n_);
  int s = ref.size();
  for (int j=0; j<s; ++j) {
    int remaining = s-j;
    int k = (n_ < remaining ? n_ : remaining);
    ngram.clear();
    for (int i=1; i<=k; ++i) {
      ngram.push_back(ref[j + i - 1]);
      tc[ngram].first++;
    }
  }

  // clipping
  for (NGramCountMap::iterator i = tc.begin(); i != tc.end(); ++i) {
    pair<int,int>& p = ngrams[i->first];
    if (p.first < i->second.first)
      p = i->second;
  }
}

float SentenceBLEU::ComputeGain(const vector<const Factor*>& sent) const {
  NGramCountMap sentNgrams;
  CountRef(sent, sentNgrams);
  
  return CalcScore(ngrams_, sentNgrams, sent.size());
}

void SentenceBLEU::GetSufficientStats(const vector<const Factor*>& sent, SufficientStats* stats) const {
  NGramCountMap sentNgrams;
  CountRef(sent, sentNgrams);
  CalcSufficientStats(ngrams_, sentNgrams, sent.size(), *(static_cast<BleuSufficientStats*>(stats)));
}
  
  
float SentenceBLEU::CalcScore(const NGramCountMap & refNgrams, const NGramCountMap & hypNgrams, int hypLen) const {
  BleuSufficientStats stats(n_);
  CalcSufficientStats(refNgrams, hypNgrams, hypLen, stats);
  return CalcBleu(stats, true, _use_bp_denum_hack, _bp_scale);
}
  
void SentenceBLEU::CalcSufficientStats(const NGramCountMap & refNgrams, const NGramCountMap & hypNgrams, int hypLen, BleuSufficientStats &stats) const {
    
  stats.ref_len = GetClosestLength(hypLen);
  stats.hyp_len = hypLen;
    
  for (NGramCountMap::const_iterator hypIt = hypNgrams.begin();
         hypIt != hypNgrams.end(); ++hypIt) {

    NGramCountMap::iterator refIt = ngrams_.find(hypIt->first);
      
    if(refIt != refNgrams.end())  {
      stats.correct[hypIt->first.size() - 1] += min(refIt->second.first, hypIt->second.first); 
    }
      
    stats.hyp[hypIt->first.size() - 1] += hypIt->second.first;
  }
  stats.src_len = m_src_len;
  //cerr << stats << endl;
}
  
float SentenceBLEU::ComputeGain(const GainFunction& hyp) const {
  assert(hyp.GetType() == this->GetType());
  const NGramCountMap& hyp_ngrams = static_cast<const SentenceBLEU&>(hyp).GetNgrams();

  return CalcScore(ngrams_, hyp_ngrams, (int) static_cast<const SentenceBLEU&>(hyp).GetAverageReferenceLength());
}  


float SentenceBLEU::CalcBleu(const BleuSufficientStats & stats, bool smooth, bool _use_bp_denum_hack, float _bp_scale) {
  assert(stats.correct.size() ==  stats.hyp.size());
  
  float smoothing_constant = 0.0;
  if (smooth)
    smoothing_constant = SMOOTHING_CONSTANT;
  
  float log_bleu = 0;
  int count = 0;
  for (size_t i = 0; i < stats.correct.size() ; ++i) {
    if (true || stats.hyp[i] > 0) {
      float lprec = log(smoothing_constant + stats.correct[i]) - log(smoothing_constant + stats.hyp[i]);
      log_bleu += lprec;
      ++count;
    }
  }
  log_bleu /= static_cast<float>(count);
  float lbp = 0.0;
    
  float bp_denum = stats.hyp_len;
    
  if (_use_bp_denum_hack)
    bp_denum = BP_DENUM_HACK;
    
  if (stats.hyp_len < stats.ref_len)
    lbp = (stats.hyp_len - stats.ref_len) / bp_denum;
  log_bleu += lbp * _bp_scale;
  
  return exp(log_bleu);
}  

float SentenceBLEU::CalcBleu(const BleuSufficientStats & stats, const BleuSufficientStats& smooth) {
  assert(stats.correct.size() ==  stats.hyp.size());
  
  float log_bleu = 0;
  int count = 0;
  for (size_t i = 0; i < stats.correct.size() ; ++i) {
    if (true || stats.hyp[i] > 0) {
      float lprec = log(smooth.correct[i] + stats.correct[i]) - log(smooth.hyp[i] + stats.hyp[i]);
      log_bleu += lprec;
      ++count;
    }
  }
  
  log_bleu /= static_cast<float>(count);
  float lbp = 0.0;
    
  float hyp_len = stats.hyp_len + smooth.hyp_len;
  float ref_len = stats.ref_len + smooth.ref_len;
    
  if (hyp_len < ref_len)
    lbp = (hyp_len - ref_len) / hyp_len;
  log_bleu += lbp ;
  
  float bleu = exp(log_bleu);
  
  if (computeApproxDocBLEU) {
    bleu *= (smooth.src_len + stats.src_len);  
  }
  else {
    bleu *= 100; 
  }
  
  return bleu;
}  
  
void SentenceBLEU::UpdateSmoothing(SufficientStats* smooth) {
   cerr << "Curr Smoothing stats : " << m_currentSmoothing << endl;
   m_currentSmoothing += *static_cast<BleuSufficientStats*>(smooth);
   m_currentSmoothing *= 0.9;
   cerr << "Now Smoothing stats : " << m_currentSmoothing << endl;
}

//#ifdef MPI_ENABLED
void UpdateSmoothing(int rank) {
  vector <float> smoothingStats(m_currentSmoothing.data().size());
  MPI_VERBOSE(1,"Before update, Rank " << rank << ", smoothing stats : " << m_currentSmoothing << endl)  
  //Reduce smoothing stats
  //if (MPI_SUCCESS != MPI_Reduce(const_cast<float*>(&m_currentSmoothing.data()[0]), &smoothingStats[0], smoothingStats.size(), MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1);
  
  //Broadcast it
  //if (MPI_SUCCESS != MPI_Bcast(const_cast<float*>(&smoothingStats[0]), smoothingStats.size(), MPI_FLOAT, 0, MPI_COMM_WORLD)) MPI_Abort(MPI_COMM_WORLD,1); 
  
  //Now unpack
  m_currentSmoothing.initialise(smoothingStats);
  MPI_VERBOSE(1,"After update, Rank " << rank << ", smoothing stats : " << m_currentSmoothing << endl)  
}
  
//#endif  
  
}


