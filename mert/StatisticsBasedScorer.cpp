//
//  StatisticsBasedScorer.cpp
//  mert_lib
//
//  Created by Hieu Hoang on 23/06/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "StatisticsBasedScorer.h"

using namespace std;

namespace MosesTuning
{


StatisticsBasedScorer::StatisticsBasedScorer(const string& name, const string& config)
  : Scorer(name,config)
{
  //configure regularisation
  static string KEY_TYPE = "regtype";
  static string KEY_WINDOW = "regwin";
  static string KEY_CASE = "case";
  static string TYPE_NONE = "none";
  static string TYPE_AVERAGE = "average";
  static string TYPE_MINIMUM = "min";
  static string TRUE = "true";
  static string FALSE = "false";

  string type = getConfig(KEY_TYPE,TYPE_NONE);
  if (type == TYPE_NONE) {
    m_regularization_type = NONE;
  } else if (type == TYPE_AVERAGE) {
    m_regularization_type = AVERAGE;
  } else if (type == TYPE_MINIMUM) {
    m_regularization_type = MINIMUM;
  } else {
    throw runtime_error("Unknown scorer regularisation strategy: " + type);
  }
  //    cerr << "Using scorer regularisation strategy: " << type << endl;

  const string& window = getConfig(KEY_WINDOW, "0");
  m_regularization_window = atoi(window.c_str());
  //    cerr << "Using scorer regularisation window: " << m_regularization_window << endl;

  const string& preserve_case = getConfig(KEY_CASE,TRUE);
  if (preserve_case == TRUE) {
    m_enable_preserve_case = true;
  } else if (preserve_case == FALSE) {
    m_enable_preserve_case = false;
  }
  //    cerr << "Using case preservation: " << m_enable_preserve_case << endl;
}

void  StatisticsBasedScorer::score(const candidates_t& candidates, const diffs_t& diffs,
                                   statscores_t& scores) const
{
  if (!m_score_data) {
    throw runtime_error("Score data not loaded");
  }
  // calculate the score for the candidates
  if (m_score_data->size() == 0) {
    throw runtime_error("Score data is empty");
  }
  if (candidates.size() == 0) {
    throw runtime_error("No candidates supplied");
  }
  int numCounts = m_score_data->get(0,candidates[0]).size();
  vector<int> totals(numCounts);
  for (size_t i = 0; i < candidates.size(); ++i) {
    ScoreStats stats = m_score_data->get(i,candidates[i]);
    if (stats.size() != totals.size()) {
      stringstream msg;
      msg << "Statistics for (" << "," << candidates[i] << ") have incorrect "
          << "number of fields. Found: " << stats.size() << " Expected: "
          << totals.size();
      throw runtime_error(msg.str());
    }
    for (size_t k = 0; k < totals.size(); ++k) {
      totals[k] += stats.get(k);
    }
  }
  scores.push_back(calculateScore(totals));

  candidates_t last_candidates(candidates);
  // apply each of the diffs, and get new scores
  for (size_t i = 0; i < diffs.size(); ++i) {
    for (size_t j = 0; j < diffs[i].size(); ++j) {
      size_t sid = diffs[i][j].first;
      size_t nid = diffs[i][j].second;
      size_t last_nid = last_candidates[sid];
      for (size_t k  = 0; k < totals.size(); ++k) {
        int diff = m_score_data->get(sid,nid).get(k)
                   - m_score_data->get(sid,last_nid).get(k);
        totals[k] += diff;
      }
      last_candidates[sid] = nid;
    }
    scores.push_back(calculateScore(totals));
  }

  // Regularisation. This can either be none, or the min or average as described in
  // Cer, Jurafsky and Manning at WMT08.
  if (m_regularization_type == NONE || m_regularization_window <= 0) {
    // no regularisation
    return;
  }

  // window size specifies the +/- in each direction
  statscores_t raw_scores(scores);      // copy scores
  for (size_t i = 0; i < scores.size(); ++i) {
    size_t start = 0;
    if (i >= m_regularization_window) {
      start = i - m_regularization_window;
    }
    const size_t end = min(scores.size(), i + m_regularization_window + 1);
    if (m_regularization_type == AVERAGE) {
      scores[i] = score_average(raw_scores,start,end);
    } else {
      scores[i] = score_min(raw_scores,start,end);
    }
  }
}

}

