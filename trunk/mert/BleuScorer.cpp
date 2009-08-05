#include "BleuScorer.h"

const int BleuScorer::LENGTH = 4;


/**
  * count the ngrams of each type, up to the given length in the input line.
 **/
size_t BleuScorer::countNgrams(const string& line, counts_t& counts, unsigned int n) {
	vector<int> encoded_tokens;
	//cerr << line << endl;
	encode(line,encoded_tokens);
	//copy(encoded_tokens.begin(), encoded_tokens.end(), ostream_iterator<int>(cerr," "));
	//cerr << endl;
	for (size_t k = 1; k <= n; ++k) {
		//ngram order longer than sentence - no point
		if (k > encoded_tokens.size()) {
			continue;
		}
		for (size_t i = 0; i < encoded_tokens.size()-k+1; ++i) {
			vector<int> ngram;
			for (size_t j = i; j < i+k && j < encoded_tokens.size(); ++j) {
				ngram.push_back(encoded_tokens[j]);
			}
			int count = 1;
			counts_it oldcount = counts.find(ngram);
			if (oldcount != counts.end()) {
				count = (oldcount->second) + 1;
			}
			//cerr << count << endl;
			counts[ngram] = count;
			//cerr << endl;
		}
	}
	//cerr << "counted ngrams" << endl;
	//dump_counts(counts);
	return encoded_tokens.size();
}

void BleuScorer::setReferenceFiles(const vector<string>& referenceFiles) {
	//make sure reference data is clear
	_refcounts.clear();
	_reflengths.clear();
	_encodings.clear();

	//load reference data
	for (size_t i = 0; i < referenceFiles.size(); ++i) {
		TRACE_ERR("Loading reference from " << referenceFiles[i] << endl);
		ifstream refin(referenceFiles[i].c_str());
		if (!refin) {
			throw runtime_error("Unable to open: " + referenceFiles[i]);
		}
		string line;
		size_t sid = 0; //sentence counter
		while (getline(refin,line)) {
			//cerr << line << endl;
			if (i == 0) {
				counts_t* counts = new counts_t(); //these get leaked
				_refcounts.push_back(counts);
				vector<size_t> lengths;
				_reflengths.push_back(lengths);
			}
			if (_refcounts.size() <= sid) {
				throw runtime_error("File " + referenceFiles[i] + " has too many sentences");
			}
			counts_t counts;
			size_t length = countNgrams(line,counts,LENGTH);
			//for any counts larger than those already there, merge them in
			for (counts_it ci = counts.begin(); ci != counts.end(); ++ci) {
				counts_it oldcount_it = _refcounts[sid]->find(ci->first);
				int oldcount = 0;
				if (oldcount_it != _refcounts[sid]->end()) {
					oldcount = oldcount_it->second;
				}
				int newcount = ci->second;
				if (newcount > oldcount) {
					_refcounts[sid]->operator[](ci->first) = newcount;
				}
			}
			//add in the length
			_reflengths[sid].push_back(length);
			if (sid > 0 && sid % 100 == 0) {
				TRACE_ERR(".");
			}
			++sid;
		}
		TRACE_ERR(endl);
	}
}


void BleuScorer::prepareStats(size_t sid, const string& text, ScoreStats& entry) {
//	cerr << text << endl;
//	cerr << sid << endl;
	//dump_counts(*_refcounts[sid]);
		if (sid >= _refcounts.size()) {
			stringstream msg;
			msg << "Sentence id (" << sid << ") not found in reference set";
			throw runtime_error(msg.str());
		}
		counts_t testcounts;
		//stats for this line
		vector<float> stats(LENGTH*2);;
		size_t length = countNgrams(text,testcounts,LENGTH);
		//dump_counts(testcounts);
		if (_refLengthStrategy == BLEU_SHORTEST) {
			//cerr << reflengths.size() << " " << sid << endl;
			int shortest = *min_element(_reflengths[sid].begin(),_reflengths[sid].end());
			stats.push_back(shortest);
		} else if (_refLengthStrategy == BLEU_AVERAGE) {
			int total = 0;
			for (size_t i = 0; i < _reflengths[sid].size(); ++i) {
				total += _reflengths[sid][i];
			}
			float mean = (float)total/_reflengths[sid].size();
			stats.push_back(mean);
		} else if (_refLengthStrategy == BLEU_CLOSEST)  {
			int min_diff = INT_MAX;
			int min_idx = 0;
			for (size_t i = 0; i < _reflengths[sid].size(); ++i) {
				int reflength = _reflengths[sid][i];
				if (abs(reflength-(int)length) < abs(min_diff)) { //look for the closest reference
					min_diff = reflength-length;
					min_idx = i;
				}else if (abs(reflength-(int)length) == abs(min_diff)) { // if two references has the same closest length, take the shortest
                                        if (reflength < (int)_reflengths[sid][min_idx]){
                                        	min_idx = i;
					}
                                }
			}
			stats.push_back(_reflengths[sid][min_idx]);
		} else {
			throw runtime_error("Unsupported reflength strategy");
		}
		//cerr << "computed length" << endl;
		//precision on each ngram type
		for (counts_it testcounts_it = testcounts.begin();
				testcounts_it != testcounts.end(); ++testcounts_it) {
			counts_it refcounts_it = _refcounts[sid]->find(testcounts_it->first);
			int correct = 0;
			int guess = testcounts_it->second;
			if (refcounts_it != _refcounts[sid]->end()) {
				correct = min(refcounts_it->second,guess);
			}
			size_t len = testcounts_it->first.size();	
			stats[len*2-2] += correct;
			stats[len*2-1] += guess;
		}
		stringstream sout;
		copy(stats.begin(),stats.end(),ostream_iterator<float>(sout," "));
		//TRACE_ERR(sout.str() << endl);
		string stats_str = sout.str();
		entry.set(stats_str);
}

float BleuScorer::calculateScore(const vector<int>& comps) {
    //cerr << "BLEU: ";
    //copy(comps.begin(),comps.end(), ostream_iterator<int>(cerr," "));
	float logbleu = 0.0;
	for (int i = 0; i < LENGTH; ++i) {
		if (comps[2*i] == 0) {
			return 0.0;
		}
		logbleu += log(comps[2*i]) - log(comps[2*i+1]);
		
	}
	logbleu /= LENGTH;
	float brevity = 1.0 - (float)comps[LENGTH*2]/comps[1];//reflength divided by test length
	if (brevity < 0.0) {
		logbleu += brevity;
	}
    //cerr << " " << exp(logbleu) << endl;
	return exp(logbleu);
}

