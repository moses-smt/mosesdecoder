#ifndef __MERGESCORER_H__
#define __MERGESCORER_H__

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
#include "TerScorer.h"
#include "BleuScorer.h"
#include "PerScorer.h"
#include "CderScorer.h"
//#include "TERsrc/tercalc.h"
//#include "TERsrc/terAlignment.h"

using namespace std;
using namespace TERCpp;

// enum MergeReferenceLengthStrategy { MERGE_AVERAGE, MERGE_SHORTEST, MERGE_CLOSEST };


/**
 * Merge scoring.
 */
class MergeScorer: public StatisticsBasedScorer {

	public:
		MergeScorer(const string& config = "") : StatisticsBasedScorer("MERGE",config){}
		virtual void setReferenceFiles(const vector<string>& referenceFiles);
		virtual void prepareStats(size_t sid, const string& text, ScoreStats& entry);
		static const int LENGTH;
		virtual void whoami()
		{
			cerr << "I AM MergeScorer" << std::endl;
		}
// 		size_t NumberOfScores(){ cerr << "MergeScorer: " << (2 * LENGTH + 1) << endl; return (2 * LENGTH + 1); };


       protected:
/*        friend float PerScorer::calculateScore(const vector<int>& comps);
        friend float BleuScorer::calculateScore(const vector<int>& comps);
        friend float TerScorer::calculateScore(const vector<int>& comps);
        friend float CderScorer::calculateScore(const vector<int>& comps);
*/	friend class PerScorer;
        float calculateScore(const vector<int>& comps);
//        float calculateScore(const vector<float>& comps);

	private:
		string javaEnv;
		string tercomEnv;
		//no copy
		MergeScorer(const MergeScorer&);
		~MergeScorer(){};
		MergeScorer& operator=(const MergeScorer&);
		// data extracted from reference files
		vector<size_t> _reflengths;
		vector<multiset<int> > _reftokens;
		vector<vector<int> > m_references;
		string m_pid;

};

#endif  //__TERSCORER_H
