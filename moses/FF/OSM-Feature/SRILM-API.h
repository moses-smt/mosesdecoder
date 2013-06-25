#pragma once

#include <vector>

class LM;

class Api
{

	public:

	Api();
	~Api();
	void read_lm(const char *,int);
	float lm_logprobContext( const char *word, const char *const *context );
	float lm_logprobSent( const char *const *sentence );
	double contextProb(char *, int & );
	double contextProbN (std::vector <int> , int &);
	unsigned backOffLength (std::vector <int>);

	double sentProb(char *) ;
	int getLMID(char *);

	private :
		
	LM *LanguageModel;

};


