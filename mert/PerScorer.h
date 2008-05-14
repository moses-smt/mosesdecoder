#ifndef __PERSCORER_H__
#define __PERSCORER_H__

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Util.h"
#include "Scorer.h"
#include "ScoreData.h"

using namespace std;


/**
  * Implementation of position-independent word error rate. This is defined
  * as 1 - (correct - max(0,output_length - ref_length)) / ref_length
  * In fact, we ignore the " 1 - " so that it can be maximised.
 **/
class PerScorer: public Scorer {
	public:
		PerScorer() : Scorer("PER"), _preserveCase(false) {}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(int sid, const string& text, ScoreStats& entry);

		virtual void score(const candidates_t& candidates, const diffs_t& diffs,
            scores_t& scores);
		
	private:
		//no copy
		PerScorer(const PerScorer&);
		PerScorer& operator=(const PerScorer&);
				
		typedef map<string,int> encodings_t;
		typedef map<string,int>::iterator encodings_it;
		void encode(const string& line, vector<int>& encoded);

 
		bool _preserveCase;
		
		// data extracted from reference files
		vector<vector<size_t> > _reflengths;
		encodings_t _encodings;
};




#endif //__PERSCORER_H
