/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2010 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "Bleu.h"

using namespace Moses;
using namespace std;

namespace Josiah {
  
  /**
    * Extract the ngrams in the given sentence, up to the BLEU_ORDER, 
    * clipping using the existing ngrams as necessary. */
  static void ExtractNGrams(const Translation& sentence, NGramMap& ngrams) {
    NGramMap newNgrams;
    for (size_t start = 0; start < sentence.size(); ++start) {
      Translation ngram;
      for (size_t length = 1; length <= BLEU_ORDER; ++length) {
        size_t position = start + length-1;
        if (position < sentence.size()) {
          ngram.push_back(sentence[position]);
          ++newNgrams[ngram];  
        } else {
          break;
        }
      }
    }
    
    //clipping
    for (NGramMap::const_iterator i = newNgrams.begin(); i != newNgrams.end(); ++i) {
      Translation ngram = i->first;
      if (ngrams[i->first] < i->second) {
        ngrams[i->first] = i->second;
      }
    }
  }

  static BleuStats ExtractStats(const NGramMap& ref, const NGramMap& hyp) {
    /*
    cerr << "ref ngrams" << endl;
    for (NGramMap::const_iterator ref_iter = ref.begin(); ref_iter != ref.end(); ++ref_iter) { 
      const Translation& ngram = ref_iter->first;
      size_t count = ref_iter->second;
      for (size_t i = 0; i < ngram.size(); ++i) {
        cerr << *(ngram[i]) << " ";
      }
      cerr << count << endl;
    }*/
    BleuStats stats;
    for (NGramMap::const_iterator hyp_iter = hyp.begin(); hyp_iter != hyp.end(); ++hyp_iter) {
      const Translation& ngram = hyp_iter->first;
      size_t count = hyp_iter->second;
      size_t order = ngram.size();
      stats.total(order, stats.total(order) + count);
      NGramMap::const_iterator ref_iter = ref.find(ngram);
      if (ref_iter != ref.end()) {
        size_t matches = min(count, ref_iter->second);
        stats.tp(order,stats.tp(order) + matches);
      }
    }
    return stats;
  }
  
  Bleu::Bleu() : m_smoothingWeight(0) {
    for (size_t i = 1; i <= BLEU_ORDER; ++i) {
      m_smoothingStats.tp(i, BLEU_SMOOTHING);
      m_smoothingStats.total(i, BLEU_SMOOTHING);
    }
  }
  
  void Bleu::SetSmoothingWeight(float smoothingWeight) {
    m_smoothingWeight = smoothingWeight;
  }
  
  float Bleu::GetSmoothingWeight() const {
    return m_smoothingWeight;
  }
  
  GainFunctionHandle Bleu::GetGainFunction(const std::vector<size_t>& sentenceIds) {
    return GainFunctionHandle(new BleuFunction(*this,sentenceIds));
  }
  
  void Bleu::AddReferences(const std::vector<Translation>& refs, const Translation& source) {
    if (m_referenceLengths.size()) {
      assert(m_referenceLengths[0].size() == refs.size());
    }
    m_sourceLengths.push_back(source.size());
    m_referenceLengths.push_back(vector<size_t>());
    m_referenceStats.push_back(NGramMap());
    for (size_t i = 0; i < refs.size(); ++i) {
      m_referenceLengths.back().push_back(refs[i].size());
      ExtractNGrams(refs[i],m_referenceStats.back());
    }
  }
  
  const NGramMap& Bleu::GetReferenceStats(size_t sentenceId) const {
    return m_referenceStats.at(sentenceId);
  }
  
  const vector<size_t>& Bleu::GetReferenceLengths(size_t sentenceId) const {
    return m_referenceLengths.at(sentenceId);
  }
  
  float Bleu::GetAverageReferenceLength(size_t sentenceId) const {
    const vector<size_t>& lengths = GetReferenceLengths(sentenceId);
    float total = 0.0f;
    for (size_t i = 0; i < lengths.size(); ++i) {
      total += lengths[i];
    }
    return total/lengths.size();
  }
  
  size_t Bleu::GetSourceLength(size_t sentenceId) const {
    return m_sourceLengths.at(sentenceId);
  }
  
  
  const BleuStats& Bleu::GetSmoothingStats() const {
    return m_smoothingStats;
  }
  
  void Bleu::AddSmoothingStats(const BleuStats& stats) {
    //Chiang's update rule.
    if (m_smoothingWeight) {
      m_smoothingStats += stats;
      m_smoothingStats *= m_smoothingWeight;
    }
  }

  BleuFunction::BleuFunction(Bleu& bleu, const vector<size_t>& sentenceIds):
      m_stats(bleu),m_sentenceIds(sentenceIds), m_smoothingStatsCount(0), m_cachedStats(sentenceIds.size())
    {}
    
  
  float BleuFunction::Evaluate(const std::vector<Translation>& hypotheses) const {
    assert(hypotheses.size() == m_sentenceIds.size());
    BleuStats totalStats;
    for (size_t i = 0; i < hypotheses.size(); ++i) {
      if (m_cachedStats[i].first != hypotheses[i]) {
        //don't have this sentence cached
        NGramMap hypNgrams;
        ExtractNGrams(hypotheses[i], hypNgrams);
        const NGramMap& refNgrams = m_stats.GetReferenceStats(m_sentenceIds[i]);
        m_cachedStats[i] = pair<Translation,BleuStats>
              (hypotheses[i],ExtractStats(refNgrams,hypNgrams));
        //cerr << "SID " << m_sentenceIds[i] << " " << m_cachedStats[i].second << endl;
      }
      totalStats += m_cachedStats[i].second;
      float src_len = m_stats.GetSourceLength(m_sentenceIds[i]);
      float hyp_len = hypotheses[i].size();
      const vector<size_t>& ref_lens = m_stats.GetReferenceLengths(m_sentenceIds[i]);
       //closest length
      float  ref_len = ref_lens[0];
      for (size_t j = 1; j < ref_lens.size(); ++j) {
        if (abs(ref_len - hyp_len) > abs(ref_lens[j] - hyp_len)) {
          ref_len = ref_lens[j];
        }
      }
      totalStats.ref_len(totalStats.ref_len() + ref_len);
      totalStats.hyp_len(totalStats.hyp_len() + hyp_len);
      totalStats.src_len(totalStats.src_len() + src_len);
      //cerr << totalStats << endl;
    }

    float log_bleu = 0;
    const BleuStats& smoothing = m_stats.GetSmoothingStats();
    for (size_t i = 1; i <= BLEU_ORDER; ++i) {
      log_bleu = log_bleu + log(totalStats.tp(i) + smoothing.tp(i)) - 
          log(totalStats.total(i) + smoothing.total(i));
    }

    log_bleu /= BLEU_ORDER;

    float ref_len = totalStats.ref_len() + smoothing.ref_len();
    float hyp_len = totalStats.hyp_len() + smoothing.hyp_len();
    
    float bp = 0;
    if (hyp_len < ref_len) {
      bp = 1 - ref_len / hyp_len;
    }
    log_bleu += bp;
    //cerr << totalStats << endl;
    //cerr << "bleu before scale: " << exp(log_bleu);
    if (m_stats.GetSmoothingWeight()) {
      //cerr << "smoothing " << smoothing << endl;
      //cerr << "lb " << log_bleu;
      //doing approx doc bleu
      log_bleu += log(totalStats.src_len() + smoothing.src_len()) - log(hypotheses.size());
      //cerr << " " << log_bleu << endl;
    } else {
      log_bleu += log(100);
    }
    //cerr << " After " << exp(log_bleu) << endl;

    //cerr << totalStats << " " << exp(log_bleu) << endl;
    return exp(log_bleu);
  }
  
  void BleuFunction::AddSmoothingStats(size_t sentenceId, const Translation& hypothesis) {
    //Only calculating stats for one sentence
    sentenceId = m_sentenceIds[sentenceId];
    
    NGramMap hypNgrams;
    ExtractNGrams(hypothesis,hypNgrams);
    const NGramMap refNgrams = m_stats.GetReferenceStats(sentenceId);
    BleuStats smoothStats = ExtractStats(refNgrams,hypNgrams);
    smoothStats.src_len(m_stats.GetSourceLength(sentenceId));
    smoothStats.hyp_len(hypothesis.size());
    smoothStats.ref_len(m_stats.GetAverageReferenceLength(sentenceId));
    m_smoothingStats += smoothStats;
    ++m_smoothingStatsCount;
  }
  
  void BleuFunction::UpdateSmoothingStats() {
    m_smoothingStats /= m_smoothingStatsCount;
    m_stats.AddSmoothingStats(m_smoothingStats);
    m_smoothingStatsCount = 0;
    m_smoothingStats.clear();
  }


  BleuStats::BleuStats() :
    m_data(BLEU_ORDER*2+3) {}
  
  void BleuStats::clear() {
    m_data = valarray<float>(BLEU_ORDER*2+3);
  }


  float BleuStats::tp(size_t order) const {
    return m_data[order*2-2];
  }

  void BleuStats::tp(size_t order, float val) {
    m_data[order*2-2] = val;
  }

  float BleuStats::total(size_t order) const {
    return m_data[order*2-1];
  }

  void BleuStats::total(size_t order, float val) {
    m_data[order*2-1] = val;
  }

  float BleuStats::src_len() const {
    return m_data[BLEU_ORDER*2];
  }

  void BleuStats::src_len(float val) {
    m_data[BLEU_ORDER*2] = val;
  }

  float BleuStats::ref_len() const {
    return m_data[BLEU_ORDER*2+1];
  }

  void BleuStats::ref_len(float val) {
    m_data[BLEU_ORDER*2+1] = val;
  }

  float BleuStats::hyp_len() const {
    return m_data[BLEU_ORDER*2+2];
  }

  void BleuStats::hyp_len(float val) {
    m_data[BLEU_ORDER*2+2] = val;
  }

  void BleuStats::operator+=(const BleuStats&  rhs) {
    m_data += rhs.m_data;
  }

  void BleuStats::operator*=(float scalar) {
    m_data *= scalar;
  }

  void BleuStats::operator/=(float scalar) {
    m_data /= scalar;
  }

  void BleuStats::write(ostream& out) const {
    out << "{";
    for (size_t i = 0; i < m_data.size(); ++i) {
      out << m_data[i];
      if (i < m_data.size()-1) out << ",";
    }
    out << "}";
  }

  ostream& operator<<(ostream& out, const BleuStats& stats) {
    stats.write(out);
    return out;
  }
}

