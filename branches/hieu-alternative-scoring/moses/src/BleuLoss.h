//
// C++ Interface: bleuloss
//
// Description: Instantiates the loss interface by implementing a getOracleCandidate based on the BLEU metric
//
//
// Author: Abhishek Arun <s0343799@alton.inf.ed.ac.uk>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BLEULOSS_H
#define BLEULOSS_H

#include <vector>
#include <map>

class Factor;
class Phrase;

typedef const Factor* WORD_ID;
/**
@author Abhishek Arun
*/
class BleuLoss
{
private:
    int ngram_size;
    int smoothing_constant;
    std::map<std::vector< WORD_ID >, int > ref_counts;
    std::vector<double>  scores;
    void extract_ngrams(const Phrase& , std::map < std::vector< WORD_ID >,int > &);
    double calculate_score(const Phrase &, const Phrase &, std::map < std::vector< WORD_ID >,int > &);
public:
    BleuLoss(int size = 4, int smooth = 1){ngram_size = size, smoothing_constant = smooth, ref_counts.clear();}
    ~BleuLoss() {};
    int getNgramSize(){ return ngram_size;}
    int getSmoothingConstant(){ return smoothing_constant;}
    double getScore(int index) { return scores[index];}
    std::vector<double> getScores() {  return scores;}
    double getScore(const Phrase &ref, const Phrase &hyp) ;
};

#endif

