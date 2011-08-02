#ifndef __TERSCORER_H__
#define __TERSCORER_H__

// #include <stdio.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits.h>
#include "Types.h"
#include "ScoreData.h"
#include "Scorer.h"
#include "TERsrc/tercalc.h"
#include "TERsrc/terAlignment.h"

using namespace std;
using namespace TERCpp;

// enum TerReferenceLengthStrategy { TER_AVERAGE, TER_SHORTEST, TER_CLOSEST };


/**
  * Bleu scoring
 **/
class TerScorer: public StatisticsBasedScorer {
	public:
		TerScorer(const string& config = "") : StatisticsBasedScorer("TER",config){}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
		static const int LENGTH;	
		virtual void whoami() 
		{
			cerr << "I AM TerScorer" << std::endl;
		}
		size_t NumberOfScores(){ cerr << "TerScorer: " << (LENGTH + 1) << endl; return (LENGTH + 1); };
		
		
//     protected:
        float calculateScore(const vector<int>& comps);
        float calculateScore(const vector<float>& comps);
		
	private:
		string javaEnv;
		string tercomEnv;
		//no copy
		TerScorer(const TerScorer&);
		~TerScorer(){};
		TerScorer& operator=(const TerScorer&);
		// data extracted from reference files
		vector<size_t> _reflengths;
		vector<multiset<int> > _reftokens;
		vector<vector<int> > m_references;
		vector<vector<vector<int> > > m_multi_references;
		string m_pid;
  
};


#endif //__TERSCORER_H
