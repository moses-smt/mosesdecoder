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

void PerScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry) {
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

float PerScorer::calculateScore(const vector<int>& comps) {
    float denom = comps[2];
    float num = comps[0] - max(0,comps[1]-comps[2]);
    if (denom == 0) {
        //shouldn't happen!
        return 0.0;
    } else {
        return num/denom;
    }
}
