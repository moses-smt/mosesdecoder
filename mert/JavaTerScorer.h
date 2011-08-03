#ifndef __JAVATERSCORER_H__
#define __JAVATERSCORER_H__

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

using namespace std;

// enum JavaTerReferenceLengthStrategy { JAVA_TER_AVERAGE, JAVA_TER_SHORTEST, JAVA_TER_CLOSEST };


/**
  * TER scoring
 **/
class JavaTerScorer: public StatisticsBasedScorer {
	public:
		JavaTerScorer(const string& config = "") : StatisticsBasedScorer("JAVATER",config){}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
		static const int LENGTH;	
		virtual void whoami() 
		{
			cerr << "I AM a JAVA TerScorer" << std::endl;
		}
		size_t NumberOfScores(){ cerr << "TerScorer: " << (2 * LENGTH + 1) << endl; return (2 * LENGTH + 1); };
		
		
//     protected:
        float calculateScore(const vector<int>& comps);
        float calculateScore(const vector<float>& comps);
		
	private:
		string javaEnv;
		string tercomEnv;
		//no copy
		JavaTerScorer(const JavaTerScorer&);
		~JavaTerScorer(){};
		JavaTerScorer& operator=(const JavaTerScorer&);
		// data extracted from reference files
		vector<size_t> _reflengths;
		vector<multiset<int> > _reftokens;
		vector<vector<int> > m_references;
		string m_pid;
  
};


#endif //__JAVATERSCORER_H
