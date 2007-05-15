#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <stdio.h>
#include "LatticePathList.h"
#include "LatticePath.h"
#include "StaticData.h"
#include "Util.h"
#include "mbr.h"
using namespace std ;


/* Input : 
   1. a sorted  n-best list, with duplicates filtered out in the following  format 
   0 ||| amr moussa is currently on a visit to libya , tomorrow , sunday , to hold talks with regard to the in sudan . ||| 0 -4.94418 0 0 -2.16036 0 0 -81.4462 -106.593 -114.43 -105.55 -12.7873 -26.9057 -25.3715 -52.9336 7.99917 -24 ||| -4.58432

   2. a weight vector 
   3. bleu order ( default = 4)
   4. scaling factor to weigh the weight vector (default = 1.0)

   Output :
   translations that minimise the Bayes Risk of the n-best list


*/

int TABLE_LINE_MAX_LENGTH = 5000;
vector<float> weights;
float SCALE = 1.0;
int BLEU_ORDER = 4;
int SMOOTH = 1;
int DEBUG = 0;
float min_interval = 1e-4;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) {_IS.getline(_LINE, _SIZE, _DELIM); if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();}

typedef string WORD;
typedef unsigned int WORD_ID;


map<WORD, WORD_ID>  lookup;
vector< WORD > vocab;

class candidate_t{
  public:
    vector<WORD_ID> translation; 
    vector<float> features;
    int translation_size;
} ;


void usage(void)
{
    fprintf(stderr,
	    "usage: mbr -s SCALE -n BLEU_ORDER -w weights.txt -i nbest.txt"); 
}


char *strstrsep(char **stringp, const char *delim) {
  char *match, *save;
  save = *stringp;
  if (*stringp == NULL)
    return NULL;
  match = strstr(*stringp, delim);
  if (match == NULL) {
    *stringp = NULL;
    return save;
  }
  *match = '\0';
  *stringp = match + strlen(delim);
  return save;
}



vector<string> tokenize( const char input[] )
{
  vector< string > token;
  bool betweenWords = true;
  int start;
  int i=0;
  for(; input[i] != '\0'; i++)
  {
    bool isSpace = (input[i] == ' ' || input[i] == '\t');

    if (!isSpace && betweenWords)
    {
      start = i;
      betweenWords = false;
    }
    else if (isSpace && !betweenWords)
    {
      token.push_back( string( input+start, i-start ) );
      betweenWords = true;
    }
  }
  if (!betweenWords)
    token.push_back( string( input+start, i-start+1 ) );
  return token;
}

  

WORD_ID storeIfNew( WORD word )
{
  if( lookup.find( word ) != lookup.end() )
    return lookup[ word ];

  WORD_ID id = vocab.size();
  vocab.push_back( word );
  lookup[ word ] = id;
  return id;
}

int count( string input, char delim )
{
  int count = 0;
  for ( int i = 0; i < input.size(); i++){
      if ( input[i] == delim)
         count++;
  }
  return count;
}


void extract_ngrams(const vector<const Factor* >& sentence, map < vector < const Factor* >, int >  & allngrams)
{
  vector< const Factor* > ngram;
  for (int k = 0; k < BLEU_ORDER; k++)
  {
    for(int i =0; i < max((int)sentence.size()-k,0); i++)
    {
      for ( int j = i; j<= i+k; j++)
      {
        ngram.push_back(sentence[j]);
      }
      ++allngrams[ngram];
      ngram.clear();
    }
  }
}

float calculate_score(const vector< vector<const Factor*> > & sents, int ref, int hyp,  vector < map < vector < const Factor *>, int > > & ngram_stats ) {
  int comps_n = 2*BLEU_ORDER+1;
  vector<int> comps(comps_n);
  float logbleu = 0.0, brevity;
  
  int hyp_length = sents[hyp].size();

  for (int i =0; i<BLEU_ORDER;i++)
  {
    comps[2*i] = 0;
    comps[2*i+1] = max(hyp_length-i,0);
  }

  map< vector < const Factor * > ,int > & hyp_ngrams = ngram_stats[hyp] ;
  map< vector < const Factor * >, int > & ref_ngrams = ngram_stats[ref] ;

  for (map< vector< const Factor * >, int >::iterator it = hyp_ngrams.begin();
       it != hyp_ngrams.end(); it++)
  {
    map< vector< const Factor * >, int >::iterator ref_it = ref_ngrams.find(it->first);
    if(ref_it != ref_ngrams.end())
    {
      comps[2* (it->first.size()-1)] += min(ref_it->second,it->second);
    }
  }
  comps[comps_n-1] = sents[ref].size();

  if (DEBUG)
  {
    for ( int i = 0; i < comps_n; i++)
      cerr << "Comp " << i << " : " << comps[i];
  }

  for (int i=0; i<BLEU_ORDER; i++)
  {
    if (comps[0] == 0)
      return 0.0;
    if ( i > 0 )
      logbleu += log((float)comps[2*i]+SMOOTH)-log((float)comps[2*i+1]+SMOOTH);
    else
      logbleu += log((float)comps[2*i])-log((float)comps[2*i+1]);
  }
  logbleu /= BLEU_ORDER;
  brevity = 1.0-(float)comps[comps_n-1]/comps[1]; // comps[comps_n-1] is the ref length, comps[1] is the test length
  if (brevity < 0.0)
    logbleu += brevity;
  return exp(logbleu);
}



vector<const Factor*> doMBR(const LatticePathList& nBestList){
//   cerr << "Sentence " << sent << " has " << sents.size() << " candidate translations" << endl;
  float marginal = 0;

  vector<float> joint_prob_vec;
  vector< vector<const Factor*> > translations;
  float joint_prob;
  vector< map < vector <const Factor *>, int > > ngram_stats;

  LatticePathList::const_iterator iter;
  LatticePath* hyp = NULL;
	for (iter = nBestList.begin() ; iter != nBestList.end() ; ++iter)
	{
		const LatticePath &path = **iter;
    joint_prob = UntransformScore(path.GetScoreBreakdown().InnerProduct(StaticData::Instance().GetAllWeights(),StaticData::Instance().GetMBRScale()));
    marginal += joint_prob;
    joint_prob_vec.push_back(joint_prob);
    //Cache ngram counts
    map < vector < const Factor *>, int > counts;
    vector<const Factor*> translation;
    GetOutputFactors(path, translation);
    
    //TO DO
    extract_ngrams(translation,counts);
    ngram_stats.push_back(counts);
    translations.push_back(translation);
   }
   //cerr << "Marginal is " << marginal;

   vector<float> mbr_loss;
   float bleu, weightedLoss;
   float weightedLossCumul = 0;
   float minMBRLoss = 1000000;
   int minMBRLossIdx = -1;
   
   /* Main MBR computation done here */
   for (int i = 0; i < nBestList.GetSize(); i++){
       weightedLossCumul = 0;
       for (int j = 0; j < nBestList.GetSize(); j++){
            if ( i != j) {
               bleu = calculate_score(translations, j, i,ngram_stats );
               weightedLoss = ( 1 - bleu) * ( joint_prob_vec[j]/marginal);
               weightedLossCumul += weightedLoss;
               if (weightedLossCumul > minMBRLoss)
                   break;
             }
       }
       if (weightedLossCumul < minMBRLoss){
           minMBRLoss = weightedLossCumul;
           minMBRLossIdx = i;
       }
   }
//    cerr << "Min pos is : " << minMBRLossIdx << endl;
//    cerr << "Min mbr loss is : " << minMBRLoss << endl;
   /* Find sentence that minimises Bayes Risk under 1- BLEU loss */
   
   return translations[minMBRLossIdx];
   //for (int i = 0; i < best_translation.size(); i++)
   //    cout << vocab[best_translation[i]] << " " ;
   //cout << endl;
   //return best_translation;
}

void GetOutputFactors(const LatticePath &path, vector <const Factor*> &translation){
	const std::vector<const Hypothesis *> &edges = path.GetEdges();
	const std::vector<FactorType>& outputFactorOrder = StaticData::Instance().GetOutputFactorOrder();
	assert (outputFactorOrder.size() == 1);

	// print the surface factor of the translation
	for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--)
	{
		const Hypothesis &edge = *edges[currEdge];
		const Phrase &phrase = edge.GetCurrTargetPhrase();
		size_t size = phrase.GetSize();
		for (size_t pos = 0 ; pos < size ; pos++)
		{
			
			const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);
			translation.push_back(factor);
		}
	}
}

/*
void read_nbest_data(string fileName)
{

   FILE * fp;
   fp = fopen (fileName.c_str() , "r");

   static char buf[10000];
   char *rest, *tok;
   int field;
   int sent_i, cur_sent;
   candidate_t *cand = NULL;
   vector<candidate_t*> testsents;
   
   cur_sent = -1;
   
    while (fgets(buf, sizeof(buf), fp) != NULL) {
	field = 0;
	rest = buf;
	while ((tok = strstrsep(&rest, "|||")) != NULL) {
            if (field == 0) {
		sent_i = strtol(tok, NULL, 10);
                cand = new candidate_t;
	    } else if (field == 2) {
                   vector<float> features;
                   char * subtok;
                   subtok = strtok (tok," ");

  		   while (subtok != NULL)
                   {
    		     features.push_back(atof(subtok));
    		     subtok = strtok (NULL, " ");
  	           }
                   cand->features = features;
            } else if (field == 1) {
                vector<string> trans_str = tokenize(tok);
                vector<WORD_ID> trans_int;
                for (int j=0; j<trans_str.size(); j++)
                {
                    trans_int.push_back( storeIfNew( trans_str[j] ) );
                }
                cand->translation= trans_int;
                cand->translation_size = cand->translation.size();
            } else if (field == 3) {
                continue;
            } 
            else {
		fprintf(stderr, "too many fields in n-best list line\n");
	    }
            field++;
	}
        if (sent_i != cur_sent){
           if (cur_sent != - 1) {
              process(cur_sent,testsents);
           }
           cur_sent = sent_i;
           testsents.clear();
        }
    testsents.push_back(cand);
    }
    process(cur_sent,testsents);
    cerr << endl;
}*/
/*
int main(int argc, char **argv)
{

    time_t starttime = time(NULL);
    int c;
    
    string f_weight = "";
    string f_nbest = "";
    
    while ((c = getopt(argc, argv, "s:w:n:i:")) != -1) {
	switch (c) {
	case 's':
	    SCALE = atof(optarg);
	    break;
	case 'n':
	    BLEU_ORDER = atoi(optarg);
	    break;
	case 'w':
	    f_weight =  optarg;
	    break;
	case 'i':
	    f_nbest = optarg;
	    break;
	default:
	    usage();
	}
    }

    argc -= optind;
    argv += optind;

    if (argc < 2) {
	usage();
    }
       

    weights = read_weights(f_weight);    
    read_nbest_data(f_nbest);
    
    time_t endtime = time(NULL);
    cerr << "Processed data in" << (endtime-starttime) << " seconds\n";
}*/
    
