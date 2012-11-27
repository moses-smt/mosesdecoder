// $Id: dict.cpp 3677 2010-10-13 09:06:51Z bertoldi $

using namespace std;

#include "mfstream.h"
#include "mempool.h"
#include "dictionary.h"
#include "cmd.h"

#define YES   1
#define NO    0

#define END_ENUM    {   (const char*)0,  0 }



static Enum_T BooleanEnum [] = {
  {    (char*)"Yes",    YES }, 
  {    (char*)"No",     NO},
  {    (char*)"yes",    YES }, 
  {    (char*)"no",     NO},
  {    (char*)"y",    YES }, 
  {    (char*)"n",     NO},
  END_ENUM
};


int main(int argc, char **argv)
{
	char *inp=NULL; 
	char *out=NULL;
	char *testfile=NULL;
	char *intsymb=NULL;  //must be single characters
	int freqflag=0;      //print frequency of words
	int sortflag=0;      //sort dictionary by frequency
	int curveflag=0;     //plot dictionary growth curve
	int curvesize=10;    //size of curve
	int listflag=0;      //print oov words in test file
	int size=1000000;    //initial size of table ....
	float load_factor=0;   //initial load factor, default LOAD_FACTOR
	
	int prunefreq=0;    //pruning according to freq value
	int prunerank=0;    //pruning according to freq rank
	
	DeclareParams((char*)
				  "InputFile", CMDSTRINGTYPE, &inp,
				  "i", CMDSTRINGTYPE, &inp,
				  "OutputFile", CMDSTRINGTYPE, &out,
				  "o", CMDSTRINGTYPE, &out,
				  "f", CMDENUMTYPE, &freqflag,BooleanEnum,
				  "Freq", CMDENUMTYPE, &freqflag,BooleanEnum,
				  "sort", CMDENUMTYPE, &sortflag,BooleanEnum,
				  "Size", CMDINTTYPE, &size,
				  "s", CMDINTTYPE, &size,
				  "LoadFactor", CMDFLOATTYPE, &load_factor,
				  "lf", CMDFLOATTYPE, &load_factor,
				  "IntSymb", CMDSTRINGTYPE, &intsymb,
				  "is", CMDSTRINGTYPE, &intsymb,
				  
				  "PruneFreq", CMDINTTYPE, &prunefreq,
				  "PruneRank", CMDINTTYPE, &prunerank,
				  "pf", CMDINTTYPE, &prunefreq,
				  "pr", CMDINTTYPE, &prunerank,
				  
				  "Curve", CMDENUMTYPE, &curveflag,BooleanEnum,
				  "c", CMDENUMTYPE, &curveflag,BooleanEnum,
				  "CurveSize", CMDINTTYPE, &curvesize,
				  "cs", CMDINTTYPE, &curvesize,
				  
				  "TestFile", CMDSTRINGTYPE, &testfile,
				  "t", CMDSTRINGTYPE, &testfile,
				  "ListOOV", CMDENUMTYPE, &listflag,BooleanEnum,
				  "oov", CMDENUMTYPE, &listflag,BooleanEnum,
				  (char*)NULL
				  );
	
	GetParams(&argc, &argv, (char*) NULL);
	
	if (inp==NULL)
    {
		std::cerr << "\nUsage: \ndict -i=inputfile [options]\n";
		std::cerr << "(inputfile can be a corpus or a dictionary)\n\n";
		std::cerr << "Options:\n";
		std::cerr << "-o=outputfile\n";
		std::cerr << "-f=[yes|no] (output word frequencies, default is false)\n";
		std::cerr << "-sort=[yes|no] (sort dictionary by frequency, default is false)\n";
		std::cerr << "-pf=<freq>  (prune words with frequency below <freq>\n";
		std::cerr << "-pr=<rank>  (prune words with frequency rank above <rank>\n";
		std::cerr << "-is= (interruption symbol) \n";
		std::cerr << "-c=[yes|no] (show dictionary growth curve)\n";
		std::cerr << "-cs=curvesize (default 10)\n";
		std::cerr << "-t=testfile (compute OOV rates on test corpus)\n";
		std::cerr << "-LoadFactor=<value> (set the load factor for cache; it should be a positive real value; if not defined a default value is used)\n";
		std::cerr << "-listOOV=[yes|no] (print OOV words to stderr, default is false)\n\n";
		
		
		exit(1);
    };
	
	// options compatibility issues:
	if (curveflag && !freqflag)
		freqflag=1;
	if (testfile!=NULL && !freqflag) {
		freqflag=1;
 		mfstream test(testfile,ios::in);
  		if (!test){
			cerr << "cannot open testfile: " << testfile << "\n";
    		exit(1);
  		}
		test.close();
		
	}
	
	//create dictionary: generating it from training corpus, or loading it from a dictionary file
        dictionary *d = new dictionary(inp,size,load_factor);

	// sort dictionary
	if (prunefreq>0 || prunerank>0 || sortflag){
		dictionary *sortd=new dictionary(d,true);
		delete d;
		d=sortd;
	}
	

	// show statistics on dictionary growth and OOV rates on test corpus
	if (testfile != NULL)
		d->print_curve(curvesize, d->test(curvesize, testfile, listflag));
	else if (curveflag)
		d->print_curve(curvesize);
	
	
	//prune words according to frequency and rank
	if (prunefreq>0 || prunerank>0){
		cerr << "pruning dictionary prunefreq:" << prunefreq << " prunerank: " << prunerank <<" \n";
		int count=0;
		int bos=d->encode(d->BoS());  
		int eos=d->encode(d->EoS());
		
		for (int i=0; i< d->size() ; i++){
			if (prunefreq && d->freq(i) <= prunefreq && i!=bos && i!=eos){
				d->freq(i,0);
				continue;
			}
			if (prunerank>0 && count>=prunerank && i!=bos && i!=eos){
				d->freq(i,0);
				continue;
			}
			count++;	
		}
	}
	// if outputfile is provided, write the dictionary into it
	if(out!=NULL) d->save(out,freqflag);
	
}

