// -*- c++ -*-
//
// This scorer measures distance between sentences in an arbitrary N-dimensional
// space on the source side.  It provides two scores for each phrase pair:
// * Distance to input, the average distance between training sentences and the
//   input sentence (are training points close to test point?)
// * Training data consistency, the average distance between training sentences
//   and their centroid (are training points close to each other?)
// Here "training sentences" refers to the subset of sentences sampled from the
// suffix array from which the phrase pair can be extracted.  The two distances
// reported as feature scores are log-transformed.
//
// This requires pre-computing the coordinates of every source sentence in the
// bitext and computing the coordinates of each input sentence at run-time.
//
// Specify the coordinates of bitext source sentences with a file called
// ${CORPUS}.${L1}.coord.gz that contains lines of space-delimited floats:
// 0.1 0.5 0.2 ...
//
// Specify the coordinates of input sentences (InputType m_coord) with XML input
// using the coord tag.  See www.statmt.org/moses/?n=Advanced.Hybrid#ntoc1 for
// turning on XML input:
// <coord coord="0.1 0.5 0.2 ..." />
//
// Activate this feature with "dist=MEASURE" where MEASURE is one of:
// euc: Euclidean distance (for spaces)
// var: total variation distance (for distributions)

#pragma once
#include "sapt_pscore_base.h"
#include "mmsapt.h"

#include <boost/foreach.hpp>

namespace sapt
{
  template<typename Token>
  class
  PScoreDist : public PhraseScorer<Token>
  {
    enum Measure {
      EuclideanDistance,
      TotalVariationDistance,
    };
    boost::shared_ptr<std::vector<std::vector<float> > > m_sid_coord;
    Measure m_measure;
  public:
    PScoreDist(boost::shared_ptr<std::vector<std::vector<float> > > const& sid_coord,
        std::string const description)
    {
      this->m_index = -1;
      this->m_num_feats = 2;
      this->m_feature_names.push_back("dist-" + description + "-i");
      this->m_feature_names.push_back("dist-" + description + "-c");
      this->m_sid_coord = sid_coord;
      if (description == "euc") {
        this->m_measure = EuclideanDistance;
      } else if (description == "var") {
        this->m_measure = TotalVariationDistance;
      } else {
        UTIL_THROW2("Unknown specification \""
            << description << "\" for dist phrase scorer (one of: euc var)");
      }
    }

    void
    operator()(Bitext<Token> const& bt,
         PhrasePair<Token>& pp,
         ttasksptr const& ttask,
         std::vector<float> * dest = NULL) const
    {
      if (!dest) {
        dest = &pp.fvals;
      }
      // Coordinates of input
      std::vector<float> const& input = *(ttask->GetSource()->m_coord);
      // Coordinates of training data centroid
      std::vector<float> centroid = std::vector<float>((*m_sid_coord)[0].size());
      BOOST_FOREACH(int const sid, pp.sids) {
        std::vector<float> const& point = (*m_sid_coord)[sid];
        for (size_t i = 0; i < centroid.size(); ++i) {
          centroid[i] += point[i];
        }
      }
      for (size_t i = 0; i < centroid.size(); ++i) {
        centroid[i] /= pp.sids.size();
      }
      // Compute log-average-distance of specified type from the training points
      // to both the input sentence and training centroid (max distance with
      // float epsilon to avoid domain error)
      float input_distance = 0;
      float centroid_distance = 0;
      if (m_measure == EuclideanDistance) {
        BOOST_FOREACH(int const sid, pp.sids) {
          std::vector<float> const& point = (*m_sid_coord)[sid];
          float input_point_distance = 0;
          float centroid_point_distance = 0;
          for (size_t i = 0; i < input.size(); ++i) {
            input_point_distance += pow(input[i] - point[i], 2);
            centroid_point_distance += pow(centroid[i] - point[i], 2);
          }
          input_distance += sqrt(input_point_distance);
          centroid_distance += sqrt(centroid_point_distance);
        }
      } else if (m_measure == TotalVariationDistance) {
        BOOST_FOREACH(int const sid, pp.sids) {
          std::vector<float> const& point = (*m_sid_coord)[sid];
          float input_point_distance = 0;
          float centroid_point_distance = 0;
          for (size_t i = 0; i < input.size(); ++i) {
            input_point_distance += std::abs(input[i] - point[i]);
            centroid_point_distance += std::abs(centroid[i] - point[i]);
          }
          input_distance += input_point_distance / 2;
          centroid_distance += centroid_point_distance / 2;
        }
      }
      input_distance /= pp.sids.size();
      centroid_distance /= pp.sids.size();
      (*dest)[this->m_index] = log(std::max(input_distance, Moses::FLOAT_EPSILON));
      (*dest)[this->m_index + 1] = log(std::max(centroid_distance, Moses::FLOAT_EPSILON));
    }
  };
}
