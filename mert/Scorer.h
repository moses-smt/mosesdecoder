#ifndef __SCORER_H__
#define __SCORER_H__

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ScoreData.h"

using namespace std;

typedef vector<pair<unsigned int, unsigned int> > diff_t;
typedef vector<diff_t> diffs_t;
typedef vector<unsigned int> candidates_t;
typedef vector<float> scores_t;

class ScoreStats;

/**
  * Superclass of all scorers and dummy implementation. In order to add a new
  * scorer it should be sufficient to override prepareStats(), setReferenceFiles()
  * and score()
**/
class Scorer {
	
	public:
		
		Scorer(const string& name): _name(name), _scoreData(0),_preserveCase(false)  {}


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
                scores_t& scores) {
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
            scores_t scores;
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
		void setScoreData(ScoreData* scoreData) {
			_scoreData = scoreData;
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


#endif //__SCORER_H
