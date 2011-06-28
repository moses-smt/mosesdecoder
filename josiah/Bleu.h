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

#pragma once

#include <map>
#include <valarray>
#include <vector>

#include <boost/unordered_map.hpp>

#include "Gain.h"

#define BLEU_ORDER 4
#define BLEU_SMOOTHING 0.01

namespace Josiah {

typedef boost::unordered_map<std::vector<const Moses::Factor*>, size_t> NGramMap;

class BleuStats {
  public:
    BleuStats();
    void clear();
    float tp(size_t order) const;
    void tp(size_t order, float val);
    float total(size_t order) const;
    void total(size_t order, float val);
    float src_len() const;
    void src_len(float val);
    float ref_len() const;
    void ref_len(float val);
    float hyp_len() const;
    void hyp_len(float val);

    void operator+=(const BleuStats&  rhs);
    void operator*=(float scalar);
    void operator/=(float scalar);

    void write(std::ostream& out) const;

  private:
    std::valarray<float> m_data;
};


class Bleu : public Gain {

  public:
    Bleu();
    virtual GainFunctionHandle GetGainFunction(const std::vector<size_t>& sentenceIds);
    virtual void AddReferences(const std::vector<Translation>& refs, const Translation& source);
    virtual float GetAverageReferenceLength(size_t sentenceId) const;
    
    const NGramMap& GetReferenceStats(size_t sentenceId) const;
    const std::vector<size_t>& GetReferenceLengths(size_t sentenceId)  const;
    size_t GetSourceLength(size_t sentenceId) const;
    /** Update the overall smoothing stats */
    const BleuStats& GetSmoothingStats() const;
    /** Get the stats for smoothing the current sentence */
    void AddSmoothingStats(const BleuStats& stats);
    /** The decay constant for Chiang smoothing. Zero indicates no smoothing */
    void SetSmoothingWeight(float smoothingWeight);
    float GetSmoothingWeight() const;
    
    
  private:
    std::vector<NGramMap> m_referenceStats;
    std::vector<std::vector<size_t> > m_referenceLengths;
    std::vector<size_t> m_sourceLengths;
    BleuStats m_smoothingStats;
    float m_smoothingWeight;
    
};

class BleuFunction : public GainFunction {
  public:
    BleuFunction(Bleu& bleu, const std::vector<size_t>& sentenceIds);
    virtual float Evaluate(const std::vector<Translation>& hypotheses) const;
    /** Add the stats for this hypothesis to the smoothing stats being collected */
    virtual void AddSmoothingStats(size_t sentenceId, const Translation& hypothesis);
    /** Inform the GainFunction that we've finished with this sentence, and it can now
    update the parent's stats */
    virtual void UpdateSmoothingStats();
    

  private:
    Bleu& m_stats;
    std::vector<size_t> m_sentenceIds;
    //smoothing stats collected for this batch
    BleuStats m_smoothingStats;
    size_t m_smoothingStatsCount;
    mutable std::vector<std::pair<Translation, BleuStats> > m_cachedStats;
};




std::ostream& operator<<(std::ostream& out, const BleuStats& stats);

}
