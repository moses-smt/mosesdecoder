#pragma once

#include <map>
#include "corpus/corpus.h"
#include "moses/Factor.h"
#include "moses/Phrase.h"

namespace Moses
{
class OXLMMapper
{
public:
 OXLMMapper(const oxlm::Dict& dict);

 int convert(const Moses::Factor *factor) const;
 std::vector<int> convert(const Phrase &phrase) const;
 void convert(const std::vector<const Word*> &contextFactor, std::vector<int> &ids, int &word) const;

private:
 void add(int lbl_id, int cdec_id);

 oxlm::Dict dict;
 typedef std::map<const Moses::Factor*, int> Coll;
 Coll moses2lbl;
 int kUNKNOWN;

};

/**
 * Wraps the feature values computed from the LBL language model.
 */
struct LBLFeatures {
  LBLFeatures() : LMScore(0), OOVScore(0) {}
  LBLFeatures(double lm_score, double oov_score)
      : LMScore(lm_score), OOVScore(oov_score) {}
  LBLFeatures& operator+=(const LBLFeatures& other) {
    LMScore += other.LMScore;
    OOVScore += other.OOVScore;
    return *this;
  }

  double LMScore;
  double OOVScore;
};

}
