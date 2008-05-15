#ifndef __PERSCORER_H__
#define __PERSCORER_H__

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
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
class PerScorer: public StatisticsBasedScorer {
	public:
		PerScorer() : StatisticsBasedScorer("PER") {}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(int sid, const string& text, ScoreStats& entry);

    protected:
        
        virtual float calculateScore(const vector<int>& comps) ;
		
	private:
        
		//no copy
		PerScorer(const PerScorer&);
		PerScorer& operator=(const PerScorer&);
				
		// data extracted from reference files
		vector<size_t> _reflengths;
        vector<multiset<int> > _reftokens;
};




#endif //__PERSCORER_H
