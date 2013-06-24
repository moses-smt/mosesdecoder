#include "SRILM-API.h"
#include "Ngram.h"


Api :: Api()
{
	LanguageModel = NULL;
}

Api :: ~Api()
{
	delete LanguageModel;
}

int Api :: getLMID(char* toBeChecked)
{

    VocabString words[11];
    unsigned len = LanguageModel->vocab.parseWords(toBeChecked, words, 10);

    if (len < 1) {
      cerr << "Error: in input file!\n";
      exit(1);
    }

    VocabString last = words[len-1];
    VocabIndex index = LanguageModel->vocab.getIndex(last,LanguageModel->vocab.unkIndex());
	
    return index;	
}

double Api :: contextProbN (vector <int> numbers, int & nonWordFlag)
{

	VocabIndex wordIndex[11];
	VocabIndex last = numbers[numbers.size()-1];	
	
	int c = 0;
	//cout<<last<<endl;
	for(int i = numbers.size()-2; i>=0; i--)
	{
	  //cout<<numbers[i]<<endl;
	  wordIndex[c] = numbers[i];
	  c++;
	}

	wordIndex[c]=Vocab_None;

	//return pow(10,LanguageModel->wordProb(last,wordIndex));
	
	return LanguageModel->wordProb(last,wordIndex);
	
}

unsigned Api :: backOffLength (vector <int> numbers)
{

	VocabIndex wordIndex[11];
	VocabIndex last = numbers[numbers.size()-1];	
	unsigned length = 0;
	
	int c = 0;
	//cout<<last<<endl;
	for(int i = numbers.size()-2; i>=0; i--)
	{
	  //cout<<numbers[i]<<endl;
	  wordIndex[c] = numbers[i];
	  c++;
	}

	wordIndex[c]=Vocab_None;

	//return pow(10,LanguageModel->wordProb(last,wordIndex));
	LanguageModel->contextID(last,wordIndex,length);
	return length;
	
}

double Api :: contextProb (char * toBeChecked, int & nonWordFlag)
{


  //read_lm(languageModel,order);
  VocabString words[11];

    unsigned len = LanguageModel->vocab.parseWords(toBeChecked, words, 10);

    if (len < 1) {
      cerr << "Error: in input file!\n";
      exit(1);
    }


    VocabString last = words[len-1];

    words[len-1] = 0;
    // reverse N-gram prefix to obtain context

    VocabIndex index = LanguageModel->vocab.getIndex(last);	


	if(index == Vocab_None)
	{
		nonWordFlag=1;
		
	}

    LanguageModel->vocab.reverse( words );

    // double cost= pow(10,lm_logprobContext(last, words ));
	double cost= lm_logprobContext(last, words);

   return cost;

}

double Api :: sentProb (char * toBeChecked)
{

	
	//read_lm(languageModel,order);
	VocabString sentence[15];
	unsigned len = LanguageModel->vocab.parseWords(toBeChecked, sentence, 15);
	
	
	if (len < 1) 
	{
      		cerr << "Error: in input file!\n";
      		exit(1);
    	}
	
	//printf("%lf\n", exp(lm_logprobSent(sentence)));
	//cout<<lm_logprobSent(sentence)<<endl;
	return pow(10,lm_logprobSent(sentence));
}	

void Api :: read_lm(const char *lmFile,int order)
{
	

  setlocale(LC_CTYPE, "");
  setlocale(LC_COLLATE, "");

  Vocab *vocab = new Vocab;
   vocab->unkIsWord() = true; /* vocabulary contains unknown word tag */

  LanguageModel = new Ngram( *vocab,order );
  assert(LanguageModel != 0);
  // LanguageModel->debugme(0);

  File file( lmFile, "r" );
  if (!LanguageModel->read( file )) {
    cerr << "format error in lm file\n";
    exit(1);
  }

  file.close();


}

float Api :: lm_logprobSent( const VocabString *sentence )

{
  TextStats obj;
  return LanguageModel->sentenceProb(sentence, obj);
}


float Api :: lm_logprobContext( const VocabString word, const VocabString *context )
{
  return LanguageModel->wordProb( word, context );  
}


