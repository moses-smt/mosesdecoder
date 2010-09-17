#ifndef __PERMUTATIONSCORER_H__
#define __PERMUTATIONSCORER_H__

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
#include "Permutation.h"

/**
  * Permutation 
 **/
class PermutationScorer: public SentenceLevelScorer 
{

    public:
        PermutationScorer(const string &distanceMetric = "HAMMING", 
                          const string &config = string());
        void setReferenceFiles(const vector<string>& referenceFiles);
        void prepareStats(size_t sid, const string& text, ScoreStats& entry);
        static const int SCORE_PRECISION;
   
        size_t NumberOfScores() const { 
				    //cerr << "PermutationScorer number of scores: 1" << endl; 
						return 1; 
				};     
        bool useAlignment() const {
				  //cout << "PermutationScorer::useAlignment returning true" << endl;
				  return true;
				};
        
    protected:
        statscore_t calculateScore(const vector<statscore_t>& scores);
        PermutationScorer(const PermutationScorer&);
        ~PermutationScorer(){};
        PermutationScorer& operator=(const PermutationScorer&);
        int getNumberWords (const string & line) const;

        distanceMetricReferenceChoice_t m_refChoiceStrategy;
        distanceMetric_t m_distanceMetric;
        
        // data extracted from reference files
        // A vector of permutations for each reference file
        vector< vector<Permutation> > m_referencePerms;
        vector<size_t> m_sourceLengths;
        vector<string> m_referenceAlignments;
        
    private:
};
//TODO need to read in floats for scores - necessary for selecting mean reference strategy and for BLEU?


#endif //__PERMUTATIONSCORER_H

