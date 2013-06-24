#pragma once

#include "Ngram.h"
#include <vector>

using namespace std;

class Api
{

	public:

	Api();
	~Api();
	void read_lm(const char *,int);
	float lm_logprobContext( const VocabString word, const VocabString *context );
	float lm_logprobSent( const VocabString *sentence );
	double contextProb(char *, int & );
	double contextProbN (std::vector <int> , int &);
	unsigned backOffLength (std::vector <int>);

	double sentProb(char *) ;
	int getLMID(char *);

	private :
		
	LM *LanguageModel;

};


