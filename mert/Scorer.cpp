#include "Scorer.h"



void  StatisticsBasedScorer::score(const candidates_t& candidates, const diffs_t& diffs,
            scores_t& scores) {
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
	vector<int> totals(numCounts);
	for (size_t i = 0; i < candidates.size(); ++i) {
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
            size_t last_nid = last_candidates[sid];
            for (size_t k  = 0; k < totals.size(); ++k) {
                int diff = _scoreData->get(sid,nid).get(k)
                    - _scoreData->get(sid,last_nid).get(k);
                totals[k] += diff;
            }
            last_candidates[sid] = nid;
        }
        scores.push_back(calculateScore(totals));
    }

}

