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
#include <unistd.h>

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
vector<double> weights;
float SCALE = 1.0;
int BLEU_ORDER = 4;
int SMOOTH = 1;
int DEBUG = 0;
double min_interval = 1e-4;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) {_IS.getline(_LINE, _SIZE, _DELIM); if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();}

typedef string WORD;
typedef unsigned int WORD_ID;


map<WORD, WORD_ID>  lookup;
vector< WORD > vocab;

class candidate_t{
  public:
    vector<WORD_ID> translation; 
    vector<double> features;
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

double calculate_probability(const vector<double> & feats, const vector<double> & weights,double SCALE){

    if (feats.size() != weights.size()) 
        cerr << "ERROR : Number of features <> number of weights " << endl;   

    double prob = 0;
    for ( int i = 0; i < feats.size(); i++){
        prob += feats[i]*weights[i]*SCALE;
    }
    return exp(prob);
}

void extract_ngrams(const vector<WORD_ID>& sentence, map < vector < WORD_ID>, int >  & allngrams)
{
  vector< WORD_ID> ngram;
  for (int k = 0; k< BLEU_ORDER; k++)
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


double calculate_score(const vector<candidate_t*> & sents, int ref, int hyp,  vector < map < vector < WORD_ID>, int > > & ngram_stats ) {
  int comps_n = 2*BLEU_ORDER+1;
  int comps[comps_n];
  double logbleu = 0.0, brevity;
  
  int hyp_length = sents[hyp]->translation_size;

  for (int i =0; i<BLEU_ORDER;i++)
  {
    comps[2*i] = 0;
    comps[2*i+1] = max(hyp_length-i,0);
  }

  map< vector < WORD_ID > ,int > & hyp_ngrams = ngram_stats[hyp] ;
  map< vector < WORD_ID >, int > & ref_ngrams = ngram_stats[ref] ;

  for (map< vector< WORD_ID >, int >::iterator it = hyp_ngrams.begin();
       it != hyp_ngrams.end(); it++)
  {
    map< vector< WORD_ID >, int >::iterator ref_it = ref_ngrams.find(it->first);
    if(ref_it != ref_ngrams.end())
    {
      comps[2* (it->first.size()-1)] += min(ref_it->second,it->second);
    }
  }
  comps[comps_n-1] = sents[ref]->translation_size;

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
      logbleu += log(static_cast<double>(comps[2*i]+SMOOTH))-log(static_cast<double>(comps[2*i+1]+SMOOTH));
    else
      logbleu += log(static_cast<double>(comps[2*i]))-log(static_cast<double>(comps[2*i+1]));
  }
  logbleu /= BLEU_ORDER;
  brevity = 1.0-(double)comps[comps_n-1]/comps[1]; // comps[comps_n-1] is the ref length, comps[1] is the test length
  if (brevity < 0.0)
    logbleu += brevity;
  return exp(logbleu);
}

vector<double> read_weights(string fileName){
  ifstream inFile;
  inFile.open(fileName.c_str());
  istream *inFileP = &inFile;

  char line[TABLE_LINE_MAX_LENGTH];
  int i=0;
  vector<double> weights;

  while(true)
  {
    i++;
    SAFE_GETLINE((*inFileP), line, TABLE_LINE_MAX_LENGTH, '\n');
    if (inFileP->eof()) break;
    vector<string> token = tokenize(line);
    
    for (int j = 0; j < token.size(); j++){
         weights.push_back(atof(token[j].c_str()));
    } 
  }
  cerr << endl;
  return weights;
}

int find_pos_of_min_element(const vector<double>& vec){

    int min_pos = -1;
    double min_element = 10000;
    for ( int i = 0; i < vec.size(); i++){
          if (vec[i] < min_element){
              min_element = vec[i];
              min_pos = i;
          }
    }
/*    cerr << "Min pos is : " << min_pos << endl;
    cerr << "Min mbr loss is : " << min_element << endl;*/
    return min_pos; 
}

void process(int sent, const vector<candidate_t*> & sents){
//   cerr << "Sentence " << sent << " has " << sents.size() << " candidate translations" << endl;
   double marginal = 0;

   vector<double> joint_prob_vec;
   double joint_prob;
   vector< map < vector <WORD_ID>, int > > ngram_stats;

   for (int i = 0; i < sents.size(); i++){
//         cerr << "Sents " << i << " has trans : " << sents[i]->translation << endl;
        //Calculate marginal and cache the posteriors
        joint_prob = calculate_probability(sents[i]->features,weights,SCALE);
        marginal += joint_prob;
        joint_prob_vec.push_back(joint_prob);
        //Cache ngram counts
        map < vector <WORD_ID>, int > counts;
        extract_ngrams(sents[i]->translation,counts);
        ngram_stats.push_back(counts);
   }
   //cerr << "Marginal is " << marginal;

   vector<double> mbr_loss;
   double bleu, weightedLoss;
   double weightedLossCumul = 0;
   double minMBRLoss = 1000000;
   int minMBRLossIdx = -1;
   
   /* Main MBR computation done here */
   for (int i = 0; i < sents.size(); i++){
       weightedLossCumul = 0;
       for (int j = 0; j < sents.size(); j++){
            if ( i != j) {
               bleu = calculate_score(sents, j, i,ngram_stats );
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
   vector<  WORD_ID > best_translation = sents[minMBRLossIdx]->translation;
   for (int i = 0; i < best_translation.size(); i++)
       cout << vocab[best_translation[i]] << " " ;
   cout << endl;
}


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
                   vector<double> features;
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
}

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
}
    
