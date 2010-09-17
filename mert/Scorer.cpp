#include "Scorer.h"

//regularisation strategies
static float score_min(const statscores_t& scores, size_t start, size_t end) {
   float min = numeric_limits<float>::max(); 
   for (size_t i = start; i < end; ++i) {
       if (scores[i] < min) {
           min = scores[i];
       }
   }
   return min;
}

static float score_average(const statscores_t& scores, size_t start, size_t end) {
    if ((end - start) < 1) {
        //shouldn't happen
        return 0;
    }
    float total = 0;
    for (size_t j = start; j < end; ++j) {
        total += scores[j];
    }

    return total / (end - start);
}

void  StatisticsBasedScorer::score(const candidates_t& candidates, const diffs_t& diffs,
            statscores_t& scores) {
    //cout << "*******StatisticsBasedScorer::score" << endl;
		if (!_scoreData) {
			throw runtime_error("Score data not loaded");
		}
    //calculate the score for the candidates
    if (_scoreData->size() == 0) {
        throw runtime_error("Score data is empty");
    }
    if (candidates.size() == 0) {
        throw runtime_error("No candidates supplied");
    }
    int numCounts = _scoreData->get(0,candidates[0]).size();
		vector<float> totals(numCounts);
		for (size_t i = 0; i < candidates.size(); ++i) {
			//cout << " i " << i << " candidates[i] " << candidates[i] << endl;
			ScoreStats stats = _scoreData->get(i,candidates[i]);
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
    //apply each of the diffs, and get new scores
    for (size_t i = 0; i < diffs.size(); ++i) {
        for (size_t j = 0; j < diffs[i].size(); ++j) {
            size_t sid = diffs[i][j].first;
            size_t nid = diffs[i][j].second;
//cout << "STSC:sid = " << sid << endl;
//cout << "STSC:nid = " << nid << endl;
            size_t last_nid = last_candidates[sid];
//cout << "STSC:oid = " << last_nid << endl;
            for (size_t k  = 0; k < totals.size(); ++k) {
                float diff = _scoreData->get(sid,nid).get(k)
                    - _scoreData->get(sid,last_nid).get(k);
                totals[k] += diff;
//cout << "STSC:nid = " << _scoreData->get(sid,nid).get(k) << endl;
//cout << "STSC:oid = " << _scoreData->get(sid,last_nid).get(k) << endl;
//cout << "STSC:diff = " << diff << endl;
//cout << "STSC:totals = " << totals[k] << endl;
            }
            last_candidates[sid] = nid;
        }
        scores.push_back(calculateScore(totals));
    }

    //regularisation. This can either be none, or the min or average as described in
    //Cer, Jurafsky and Manning at WMT08
    if (_regularisationStrategy == REG_NONE || _regularisationWindow <= 0) {
        //no regularisation
        return;
    }

    //window size specifies the +/- in each direction
    statscores_t raw_scores(scores);//copy scores
    for (size_t i = 0; i < scores.size(); ++i) {
        size_t start = 0;
        if (i >= _regularisationWindow) {
            start = i - _regularisationWindow;
        }
        size_t end = min(scores.size(), i + _regularisationWindow+1);
        if (_regularisationStrategy == REG_AVERAGE) {
            scores[i] = score_average(raw_scores,start,end);
        } else {
            scores[i] = score_min(raw_scores,start,end);
        }
    }
}






/** The sentence level scores have already been calculated, just need to average them
    and include the differences. Allows scores which are floats **/
void  SentenceLevelScorer::score(const candidates_t& candidates, const diffs_t& diffs,
            statscores_t& scores) {
    //cout << "*******SentenceLevelScorer::score" << endl;
    if (!_scoreData) {
      throw runtime_error("Score data not loaded");
    }
    //calculate the score for the candidates
    if (_scoreData->size() == 0) {
        throw runtime_error("Score data is empty");
    }
    if (candidates.size() == 0) {
        throw runtime_error("No candidates supplied");
    }
    int numCounts = _scoreData->get(0,candidates[0]).size();
    vector<float> totals(numCounts);
    for (size_t i = 0; i < candidates.size(); ++i) {
        //cout << " i " << i << " candi " << candidates[i] ;
        ScoreStats stats = _scoreData->get(i,candidates[i]);
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
            size_t sid = diffs[i][j].first;
            size_t nid = diffs[i][j].second;
//cout << "sid = " << sid << endl;
//cout << "nid = " << nid << endl;
            size_t last_nid = last_candidates[sid];
            for (size_t k  = 0; k < totals.size(); ++k) {
                float diff = _scoreData->get(sid,nid).get(k)
                    - _scoreData->get(sid,last_nid).get(k);
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
    if (_regularisationStrategy == REG_NONE || _regularisationWindow <= 0) {
        //no regularisation
        return;
    }

    //window size specifies the +/- in each direction
    statscores_t raw_scores(scores);//copy scores
    for (size_t i = 0; i < scores.size(); ++i) {
        size_t start = 0;
        if (i >= _regularisationWindow) {
            start = i - _regularisationWindow;
        }
        size_t end = min(scores.size(), i + _regularisationWindow+1);
        if (_regularisationStrategy == REG_AVERAGE) {
            scores[i] = score_average(raw_scores,start,end);
        } else {
            scores[i] = score_min(raw_scores,start,end);
        }
    }
}


