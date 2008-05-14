#ifndef __SCORER_H__
#define __SCORER_H__

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Scorer {
	/**
	  * Extract initial statistics from the nbestfile and write them to the stats
      * file. For example, for bleu these are the ngram counts and length.
	  **/
	public:
		
		Scorer(const string& statsfile): _statsfile(statsfile) {}
		
		virtual void prepare(const vector<string>& referencefiles, const string& nbestfile) {
			//dummy impl
		}
	/**
	  * Calculate the score of the sentences corresponding to the list of candidate
      * indices. Each index indicates the 1-best choice from the n-best list.
	  **/
		virtual float score(const vector<unsigned int>& candidates) {
			//dummy impl
			return 0;
		}

	protected:
		string _statsfile;

};


#endif //__SCORER_H
