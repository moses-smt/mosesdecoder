//
//  SentenceLevelScorer.cpp
//  mert_lib
//
//  Created by Hieu Hoang on 22/06/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include "SentenceLevelScorer.h"

#include <iostream>
#include <boost/spirit/home/support/detail/lexer/runtime_error.hpp>

using namespace std;

namespace MosesTuning
{

SentenceLevelScorer::SentenceLevelScorer(const string& name, const string& config)
  : Scorer(name, config),
    m_regularisationStrategy(REG_NONE),
    m_regularisationWindow(0)
{
  Init();
}

SentenceLevelScorer::~SentenceLevelScorer() {}

void SentenceLevelScorer::Init()
{
  // Configure regularisation.
  static string KEY_TYPE = "regtype";
  static string KEY_WINDOW = "regwin";
  static string KEY_CASE = "case";
  static string TYPE_NONE = "none";
  static string TYPE_AVERAGE = "average";
  static string TYPE_MINIMUM = "min";
  static string TRUE = "true";
  static string FALSE = "false";

  const string type = getConfig(KEY_TYPE, TYPE_NONE);
  if (type == TYPE_NONE) {
    m_regularisationStrategy = REG_NONE;
  } else if (type == TYPE_AVERAGE) {
    m_regularisationStrategy = REG_AVERAGE;
  } else if (type == TYPE_MINIMUM) {
    m_regularisationStrategy = REG_MINIMUM;
  } else {
    throw boost::lexer::runtime_error("Unknown scorer regularisation strategy: " + type);
  }
  cerr << "Using scorer regularisation strategy: " << type << endl;

  const string window = getConfig(KEY_WINDOW, "0");
  m_regularisationWindow = atoi(window.c_str());
  cerr << "Using scorer regularisation window: " << m_regularisationWindow << endl;

  const string preservecase = getConfig(KEY_CASE, TRUE);
  if (preservecase == TRUE) {
    m_enable_preserve_case = true;
  } else if (preservecase == FALSE) {
    m_enable_preserve_case = false;
  }
  cerr << "Using case preservation: " << m_enable_preserve_case << endl;
}

void  SentenceLevelScorer::score(const candidates_t& candidates, const diffs_t& diffs,
                                 statscores_t& scores)
{
  //cout << "*******SentenceLevelScorer::score" << endl;
  if (!m_score_data) {
    throw runtime_error("Score data not loaded");
  }
  //calculate the score for the candidates
  if (m_score_data->size() == 0) {
    throw runtime_error("Score data is empty");
  }
  if (candidates.size() == 0) {
    throw runtime_error("No candidates supplied");
  }
  const int numCounts = m_score_data->get(0,candidates[0]).size();
  vector<float> totals(numCounts);
  for (size_t i = 0; i < candidates.size(); ++i) {
    //cout << " i " << i << " candi " << candidates[i] ;
    ScoreStats stats = m_score_data->get(i,candidates[i]);
    if (stats.size() != totals.size()) {
      stringstream msg;
      msg << "Statistics for (" << "," << candidates[i] << ") have incorrect "
          << "number of fields. Found: " << stats.size() << " Expected: "
          << totals.size();
      throw runtime_error(msg.str());
    }
    //Add up scores for all sentences, would normally be just one score
    for (size_t k = 0; k < totals.size(); ++k) {
      totals[k] += stats.get(k);
      //cout << " stats " << stats.get(k) ;
    }
    //cout << endl;
  }
  //take average
  for (size_t k = 0; k < totals.size(); ++k) {
    //cout << "totals = " << totals[k] << endl;
    //cout << "cand = " << candidates.size() << endl;
    totals[k] /= candidates.size();
    //cout << "finaltotals = " << totals[k] << endl;
  }

  scores.push_back(calculateScore(totals));

  candidates_t last_candidates(candidates);
  //apply each of the diffs, and get new scores
  for (size_t i = 0; i < diffs.size(); ++i) {
    for (size_t j = 0; j < diffs[i].size(); ++j) {
      const size_t sid = diffs[i][j].first;
      const size_t nid = diffs[i][j].second;
      //cout << "sid = " << sid << endl;
      //cout << "nid = " << nid << endl;
      const size_t last_nid = last_candidates[sid];
      for (size_t k  = 0; k < totals.size(); ++k) {
        const float diff = m_score_data->get(sid,nid).get(k)
                           - m_score_data->get(sid,last_nid).get(k);
        //cout << "diff = " << diff << endl;
        totals[k] += diff/candidates.size();
        //cout << "totals = " << totals[k] << endl;
      }
      last_candidates[sid] = nid;
    }
    scores.push_back(calculateScore(totals));
  }

  //regularisation. This can either be none, or the min or average as described in
  //Cer, Jurafsky and Manning at WMT08
  if (m_regularisationStrategy == REG_NONE || m_regularisationWindow <= 0) {
    //no regularisation
    return;
  }

  //window size specifies the +/- in each direction
  statscores_t raw_scores(scores);//copy scores
  for (size_t i = 0; i < scores.size(); ++i) {
    size_t start = 0;
    if (i >= m_regularisationWindow) {
      start = i - m_regularisationWindow;
    }
    const size_t end = min(scores.size(), i + m_regularisationWindow+1);
    if (m_regularisationStrategy == REG_AVERAGE) {
      scores[i] = score_average(raw_scores, start, end);
    } else {
      scores[i] = score_min(raw_scores, start, end);
    }
  }
}

}
