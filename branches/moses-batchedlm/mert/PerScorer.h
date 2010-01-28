#ifndef __PERSCORER_H__
#define __PERSCORER_H__

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
#include "Scorer.h"


using namespace std;

/**
  * Implementation of position-independent word error rate. This is defined
  * as 1 - (correct - max(0,output_length - ref_length)) / ref_length
  * In fact, we ignore the " 1 - " so that it can be maximised.
 **/
class PerScorer: public StatisticsBasedScorer {
	public:
		PerScorer(const string& config = "") : StatisticsBasedScorer("PER",config) {}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
				
		virtual void whoami() {
			cerr << "I AM PerScorer" << std::endl;
		}
		
		size_t NumberOfScores(){ cerr << "PerScorer: 3" << endl; return 3; };
		
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

#endif //__PERSCORER_H
