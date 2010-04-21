/*
**Remark: the sentences should not exceed 1000 words!!!!!!!!!!!**
**********************************************************
Head file ---------- DPR_reordering.h
The reordering feature function for MOSES
based on the DPR model proposed in (Ni et al., 2009)

Components:
        vector<unsigned long long> m_dprOptionStartPOS  --- store the start pos for each sentence option 
        ifstream sentenceOptionFile --- the ifstream file of the sentenceOption
        mutable long int sentenceID --- store the ID of current sentence needed translation
        mutable mapPhraseOption sentencePhraseOption --- store the phrase option for each sentence
        int classSetup --- store the number of orientations
        float unDetectProb --- the const reodering prob if the phrase pair is not in sentence option
        vector<float> WDR_cost --- the word distance reodering cost
             
Functions:
0. Constructor: DPR_reordering(ScoreIndexManager &scoreIndexManager, const std::string &filePath, const std::vector<float>& weights)
       
1. interface functions:
       GetNumScoreComponents() --- return the number of scores the component used (usually 1)
       GetScoreProducerDescription() --- return the name of the reordering model
       GetScoreProducerWeightShortName() --- return the short name of the weight for the score
2. Score producers:
       Evaluate() --- to evaluate the reordering scores and add the score to the score component collection
       EmptyHypothesisState() --- create an empty hypothesis
       
3. Other functions:
       constructSentencePhraseOption() --- Construct sentencePhraseOption using sentenceID
       clearSentencePhraseOption() --- clear the sentence phrase options
       generateReorderingProb(...) --- generate the reordering probability
       createOrientationClass(int dist) --- the create the orientation class
**********************************************************
*/
#pragma once
#ifndef DPR_REORDERING_H
#define DPR_REORDERING_H
#include <cstdlib>
#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>        //using istringstream
#include <fstream>         //using ifstream
#include <math.h>
#include "FeatureFunction.h"
#include "Hypothesis.h"
#include "WordsRange.h"
#include "StaticData.h"
#include "InputType.h"
#define MAXRATIO  5.0        //the maximum ration for the 3-class setup

#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

//********************************************************************
//The following definition is for the use of hash_map with string key
namespace __gnu_cxx
{
        template<> struct hash< std::string >
        {
                size_t operator()( const std::string& x ) const
                {
                        return hash< const char* >()( x.c_str() );
                }
        };
}
//********************************************************************

namespace std{using namespace __gnu_cxx;}
using namespace std;
using std::ifstream;
using std::istringstream;
using std::vector;
using std::string;
using std::ostringstream;
//for sentencePhraseOption
//typedef std::hash_map<unsigned short, hash_map<unsigned short, hash_map<string, vector<float> > > > mapPhraseOptionLeft;
//typedef std::hash_map<unsigned short, hash_map<string, vector<float> > > mapPhraseOptionRight;
//typedef std::map<vector<unsigned short>, map<string, vector<float> > > mapPhraseOption;
typedef std::hash_map<size_t, hash_map<string, vector<float> > > mapPhraseOption;
typedef std::hash_map<string, vector<float> > mapTargetProbOption;


namespace Moses
{
    using namespace std;
    
    //define the class DPR_reordering
    class DPR_reordering : public StatefulFeatureFunction 
    {
          public:
                 //constructor
                 DPR_reordering(ScoreIndexManager &scoreIndexManager, const string filePath, const string classString, const vector<float>& weights);
                 ~DPR_reordering();
          public:
                 //interface: include 3 functions
                 size_t GetNumScoreComponents() const; //return the number of scores the component used
                 string GetScoreProducerDescription() const; //return the name of the reordering model
                 string GetScoreProducerWeightShortName() const; //return the short name of the weight for the score
          public:
                 //The evaluation function and score calculation function 
                // FFState* Evaluate(const Hypothesis& cur_hypo, const FFState* prev_state, ScoreComponentCollection* accumulator);
								FFState* Evaluate(
																					const Hypothesis& cur_hypo,
																					const FFState* prev_state,
																					ScoreComponentCollection* accumulator) const;
			
								const FFState* EmptyHypothesisState() const;   
                 
          public:
                 void clearSentencePhraseOption() const;     //clear the sentence phrase options
                 void constructSentencePhraseOption() const;             //construct sentence phrase options (for a sentence)
                 float generateReorderingProb(size_t boundary_left, size_t boundary_right, size_t prev_boundary_right, string targetPhrase) const; //generate the reordering probability
                 int createOrientationClass(int dist) const;             //the create the orientation class
          private:
                  vector<unsigned long long> m_dprOptionStartPOS;                 //store the start pos for each sentence option 
                  ifstream sentenceOptionFile;                                   //the ifstream file of the sentenceOption
                  mutable long int sentenceID;                                                 //store the ID of current sentence needed translation
                  mutable mapPhraseOption sentencePhraseOption;                         //store the phrase option for each sentence
                  //mutable mapPhraseOptionLeft sentencePhraseOption;
                  int classSetup;                                               //store the number of orientations
                  float unDetectProb;                                   //the const reodering prob if the phrase pair is not in sentence option
                  vector<float> WDR_cost;                                   //the word distance reodering cost
		};
};
#endif
