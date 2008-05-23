#ifndef __SCORER_H__
#define __SCORER_H__

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "Types.h"
#include "ScoreData.h"

using namespace std;



class ScoreStats;

/**
  * Superclass of all scorers and dummy implementation. In order to add a new
  * scorer it should be sufficient to override prepareStats(), setReferenceFiles()
  * and score() (or calculateScore()). 
**/
class Scorer {
	
	public:
		
		Scorer(const string& name): _name(name), _scoreData(0),_preserveCase(false)  {};
		virtual ~Scorer(){};
	
		/**
		  * set the reference files. This must be called before prepareStats.
		  **/
		virtual void setReferenceFiles(const vector<string>& referenceFiles) {
			//do nothing
		}


		/**
		 * Process the given guessed text, corresponding to the given reference sindex
         * and add the appropriate statistics to the entry.
		**/
		virtual void prepareStats(int sindex, const string& text, ScoreStats& entry) {
			//cerr << text << std::endl;
		}

        /**
          * Score using each of the candidate index, then go through the diffs
          * applying each in turn, and calculating a new score each time.
          **/
        virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                statscores_t& scores) {
            //dummy impl
			if (!_scoreData) {
				throw runtime_error("score data not loaded");
			}
            scores.push_back(0);
            for (size_t i = 0; i < diffs.size(); ++i) {
                scores.push_back(0);
            }
        }


		/**
		  * Calculate the score of the sentences corresponding to the list of candidate
		  * indices. Each index indicates the 1-best choice from the n-best list.
		  **/
		float score(const candidates_t& candidates) {
            diffs_t diffs;
            statscores_t scores;
            score(candidates, diffs, scores);
            return scores[0];
		}

		const string& getName() const {return _name;}

        size_t getReferenceSize() {
            if (_scoreData) {
                return _scoreData->size();
            }
            return 0;
        }
		

		/**
		  * Set the score data, prior to scoring.
		  **/
		void setScoreData(ScoreData* data) {
			_scoreData = data;
		}

		protected:
            typedef map<string,int> encodings_t;
            typedef map<string,int>::iterator encodings_it;

            /**
              * Tokenise line and encode.
              * 	Note: We assume that all tokens are separated by single spaces
              **/
            void encode(const string& line, vector<int>& encoded) {
                //cerr << line << endl;
                istringstream in (line);
                string token;
                while (in >> token) {
                    if (!_preserveCase) {
                        for (string::iterator i = token.begin(); i != token.end(); ++i) {
                            *i = tolower(*i);
                        }
                    }
                    encodings_it encoding = _encodings.find(token);
                    int encoded_token;
                    if (encoding == _encodings.end()) {
                        encoded_token = (int)_encodings.size();
                        _encodings[token] = encoded_token;
                        //cerr << encoded_token << "(n) ";
                    } else {
                        encoded_token = encoding->second;
                        //cerr << encoded_token << " ";
                    }
                    encoded.push_back(encoded_token);
                }
                //cerr << endl;
            }


			ScoreData* _scoreData;
		    encodings_t _encodings;
		    bool _preserveCase;

		private:
			string _name;

};


/**
  * Abstract base class for scorers that work by adding statistics across all 
  * outout sentences, then apply some formula, e.g. bleu, per. **/
class StatisticsBasedScorer : public Scorer {

    public:
  StatisticsBasedScorer(const string& name): Scorer(name) {}
	~StatisticsBasedScorer(){};
	virtual void score(const candidates_t& candidates, const diffs_t& diffs,
                statscores_t& scores);

    protected:
        //calculate the actual score
        virtual statscore_t calculateScore(const vector<int>& totals) = 0;

};


enum BleuReferenceLengthStrategy { AVERAGE, SHORTEST, CLOSEST };


/**
  * Bleu scoring
 **/
class BleuScorer: public StatisticsBasedScorer {
	public:
		BleuScorer() : StatisticsBasedScorer("BLEU"),_refLengthStrategy(SHORTEST) {}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(unsigned int sid, const string& text, ScoreStats& entry);
		static const int LENGTH;	
    
    protected:
        float calculateScore(const vector<int>& comps);
		
	private:
		//no copy
		BleuScorer(const BleuScorer&);
		~BleuScorer(){};
		BleuScorer& operator=(const BleuScorer&);
				

		//Used to construct the ngram map
		struct CompareNgrams {
			int operator() (const vector<int>& a, const vector<int>& b) {
				size_t i;
				size_t as = a.size();
				size_t bs = b.size();
				for (i = 0; i < as && i < bs; ++i) {
					if (a[i] < b[i]) {
						//cerr << "true" << endl;
						return true;
					}
					if (a[i] > b[i]) {
						//cerr << "false" << endl;
						return false;
					}
				}
				//entries are equal, shortest wins
				return as < bs;;
			}
		};

		typedef map<vector<int>,int,CompareNgrams> counts_t;
		typedef map<vector<int>,int,CompareNgrams>::iterator counts_it;

		typedef vector<counts_t*> refcounts_t;

		size_t countNgrams(const string& line, counts_t& counts, unsigned int n);

		void dump_counts(counts_t& counts) {
			for (counts_it i = counts.begin(); i != counts.end(); ++i) {
				cerr << "(";
				copy(i->first.begin(), i->first.end(), ostream_iterator<int>(cerr," "));
				cerr << ") " << i->second << ", ";
			}
			cerr << endl;
		} 
		BleuReferenceLengthStrategy _refLengthStrategy;
		
		// data extracted from reference files
		refcounts_t _refcounts;
		vector<vector<size_t> > _reflengths;
};




/**
  * Implementation of position-independent word error rate. This is defined
  * as 1 - (correct - max(0,output_length - ref_length)) / ref_length
  * In fact, we ignore the " 1 - " so that it can be maximised.
 **/
class PerScorer: public StatisticsBasedScorer {
	public:
		PerScorer() : StatisticsBasedScorer("PER") {}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(unsigned int sid, const string& text, ScoreStats& entry);

    protected:
        
        virtual float calculateScore(const vector<int>& comps) ;
		
	private:
        
		//no copy
		PerScorer(const PerScorer&);
		~PerScorer(){};
		PerScorer& operator=(const PerScorer&);
				
		// data extracted from reference files
		vector<size_t> _reflengths;
        vector<multiset<int> > _reftokens;
};


class ScorerFactory {

    public:
        vector<string> getTypes() {
            vector<string> types;
            types.push_back(string("BLEU"));
            types.push_back(string("PER"));
            return types;
        }

        Scorer* getScorer(const string& type) {
            if (type == "BLEU") {
                return new BleuScorer();
            } else if (type == "PER") {
                return new PerScorer();
            } else {
                throw runtime_error("Unknown scorer type: " + type);
            }
       }
};

#endif //__SCORER_H
