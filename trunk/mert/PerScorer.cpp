#include "PerScorer.h"


void PerScorer::setReferenceFiles(const vector<string>& referenceFiles) {
    // for each line in the reference file, create a multiset of the 
    // word ids
    if (referenceFiles.size() != 1) {
        throw runtime_error("PER only supports a single reference");
    }
    _reftokens.clear();
    _reflengths.clear();
    ifstream in(referenceFiles[0].c_str());
    if (!in) {
        throw runtime_error("Unable to open " + referenceFiles[0]);
    }
    string line;
    int sid = 0;
    while (getline(in,line)) {
        vector<int> tokens;
        encode(line,tokens);
        _reftokens.push_back(multiset<int>());
        for (size_t i = 0; i < tokens.size(); ++i) {
            _reftokens.back().insert(tokens[i]);
        }
        _reflengths.push_back(tokens.size());
        if (sid > 0 && sid % 100 == 0) {
            TRACE_ERR(".");
        }
        ++sid;
    }
    TRACE_ERR(endl);

}

void PerScorer::prepareStats(int sid, const string& text, ScoreStats& entry) {
    if (sid >= _reflengths.size()) {
        stringstream msg;
        msg << "Sentence id (" << sid << ") not found in reference set";
        throw runtime_error(msg.str());
    }
    //calculate correct, output_length and ref_length for 
    //the line and store it in entry
    vector<int> testtokens;
    encode(text,testtokens);
    multiset<int> testtokens_all(testtokens.begin(),testtokens.end());
    set<int> testtokens_unique(testtokens.begin(),testtokens.end());
    int correct = 0;
    for (set<int>::iterator i = testtokens_unique.begin();
        i != testtokens_unique.end(); ++i) {
        int token = *i;
        correct += min(_reftokens[sid].count(token), testtokens_all.count(token));
    }
     
    ostringstream stats;
    stats << correct << " " << testtokens.size() << " " << _reflengths[sid] << " " ;
    string stats_str = stats.str();
    entry.set(stats_str);
}

float PerScorer::per(const vector<int>& comps) const {
    float denom = comps[2];
    float num = comps[0] - max(0,comps[1]-comps[2]);
    if (denom == 0) {
        //shouldn't happen!
        return 0.0;
    } else {
        return num/denom;
    }
}

void PerScorer::score(const candidates_t& candidates, const diffs_t& diffs,
            scores_t& scores) {
    //calculate the PER
  /* Implementation of position-independent word error rate. This is defined
  * as 1 - (correct - max(0,output_length - ref_length)) / ref_length
  * In fact, we ignore the " 1 - " so that it can be maximised.
  */
  //TODO: This code is pretty much the same as bleu. Could factor it out into 
  //a common superclass
    if (!_scoreData) {
		throw runtime_error("score data not loaded");
	}
    //calculate the score for the candidates
	vector<int> comps(3); //correct, output, ref
	for (size_t i = 0; i < candidates.size(); ++i) {
		ScoreStats stats = _scoreData->get(i,candidates[i]);
		if (stats.size() != comps.size()) {
			stringstream msg;
			msg << "PER statistics for (" << "," << candidates[i] << ") have incorrect "
				<< "number of fields. Found: " << stats.size() << " Expected: " 
				<< comps.size();
			throw runtime_error(msg.str());
		}
		for (size_t k = 0; k < comps.size(); ++k) {
			comps[k] += stats.get(k);	
		}
	}
    scores.push_back(per(comps));

    candidates_t last_candidates(candidates);
    //apply each of the diffs, and get new scores
    for (size_t i = 0; i < diffs.size(); ++i) {
        for (size_t j = 0; j < diffs[i].size(); ++j) {
            size_t sid = diffs[i][j].first;
            size_t nid = diffs[i][j].second;
            size_t last_nid = last_candidates[sid];
            for (size_t k  = 0; k < comps.size(); ++k) {
                int diff = _scoreData->get(sid,nid).get(k)
                    - _scoreData->get(sid,last_nid).get(k);
                comps[k] += diff;
            }
            last_candidates[sid] = nid;
        }
        scores.push_back(per(comps));
    }
}
		

