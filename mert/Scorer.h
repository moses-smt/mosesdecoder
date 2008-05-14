#ifndef __SCORER_H__
#define __SCORER_H__

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>


class ScoreData;
class ScoreStats;

class Scorer {
	
	public:
		
		Scorer(const std::string& name): _name(name), _scoreData(0)  {}

		const std::string& getName() const {return _name;}

		/**
		  * set the reference files. This must be called before prepareStats.
		  **/
		void setReferenceFiles(const std::vector<std::string>& referenceFiles) {
			_referenceFiles.clear();
			_referenceFiles.resize(referenceFiles.size());
			std::copy(referenceFiles.begin(),referenceFiles.end(),_referenceFiles.begin());
		}
		
		/**
		 * Process the given guessed text, corresponding to the given reference sindex
         * and add the appropriate statistics to the entry.
		**/
		virtual void prepareStats(int sindex, const std::string& text, ScoreStats& entry) {
			//std::cerr << text << std::endl;
		}

		/**
		  * Set the score data, prior to scoring.
		  **/
		void setScoreData(ScoreData* scoreData) {
			_scoreData = scoreData;
		}

		/**
		  * Calculate the score of the sentences corresponding to the list of candidate
		  * indices. Each index indicates the 1-best choice from the n-best list.
		  **/
		virtual float score(const std::vector<unsigned int>& candidates) {
			if (!_scoreData) {
				throw std::runtime_error("score data not loaded");
			}
			return 0;
		}

		private:
			std::string _name;
			std::vector<std::string> _referenceFiles;
			ScoreData* _scoreData;

};


#endif //__SCORER_H
