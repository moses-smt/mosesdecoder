/******************************************************************************
IrstLM: IRST Language Model Toolkit
Copyright (C) 2006 Marcello Federico, ITC-irst Trento, Italy

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

******************************************************************************/

using namespace std;

#include <cmath>
#include <math.h>
#include "mfstream.h"
#include "mempool.h"
#include "htable.h"
#include "dictionary.h"
#include "n_gram.h"
#include "mempool.h"
#include "ngramtable.h"
#include "interplm.h"
#include "normcache.h"
#include "ngramcache.h"
#include "mdiadapt.h"
#include "shiftlm.h"
#include "linearlm.h"
#include "mixture.h"
#include "cmd.h"
#include "lmtable.h"

#define YES   1
#define NO    0


#define NGRAM 1
#define SEQUENCE 2
#define ADAPT 3
#define TURN 4
#define TEXT 5

#define END_ENUM    {   (char*)0,  0 }

static Enum_T BooleanEnum [] = {
  {    "Yes",    YES }, 
  {    "No",     NO},
  {    "yes",    YES }, 
  {    "no",     NO},
  {    "y",     YES }, 
  {    "n",     NO},
  END_ENUM
};

static Enum_T LmTypeEnum [] = {
  {    "ModifiedShiftBeta",  MOD_SHIFT_BETA }, 
  {    "msb",                MOD_SHIFT_BETA }, 
  {    "InterpShiftBeta",    SHIFT_BETA }, 
  {    "ShiftBeta",          SHIFT_BETA }, 
  {    "sb",                 SHIFT_BETA }, 
  {    "InterpShiftOne",     SHIFT_ONE },
  {    "ShiftOne",           SHIFT_ONE }, 
  {    "s1",                 SHIFT_ONE }, 
  {    "LinearWittenBell",   LINEAR_WB },
  {    "wb",                 LINEAR_WB },
  {    "LinearGoodTuring",   LINEAR_GT },
  {    "Mixture",            MIXTURE },
  {    "mix",                MIXTURE },
  END_ENUM
};


static Enum_T InteractiveModeEnum [] = {
  {    "Ngram",       NGRAM }, 
  {    "Sequence",    SEQUENCE },
  {    "Adapt",       ADAPT },
  {    "Turn",        TURN },
  {    "Text",        TEXT },
  {    "Yes",         NGRAM }, 
  END_ENUM
};


int main(int argc, char **argv){
	
	char *dictfile=NULL; 
	char *trainfile=NULL; 
	char *testfile=NULL;
	char *adaptfile=NULL;
	char *slminfo=NULL;
	
	char *imixpar=NULL;
	char *omixpar=NULL;
	
	char *BINfile=NULL;
	char *ARPAfile=NULL;
	char *ASRfile=NULL;
	
	int backoff=0; //back-off or interpolation
	int lmtype=0;
	int dub=0; //dictionary upper bound
	int size=0;   //lm size

	int interactive=0;
	int statistics=0;
	
	int prunefreq=NO;
	int prunesingletons=YES;
	int prunetopsingletons=NO;
	
	double beta=-1;
	
	int compsize=NO;
	int checkpr=NO;
	double oovrate=0;
	int max_caching_level=0;
	
	char *outpr=NULL;
	
	int memmap = 0; //write binary format with/without memory map, default is 0
	
	int adaptlevel=0;   //adaptation level
	double adaptrate=1.0;
	int adaptoov=0; //do not increment the dictionary
	
	DeclareParams(
				  
				  "Back-off",CMDENUMTYPE, &backoff, BooleanEnum,
				  "bo",CMDENUMTYPE, &backoff, BooleanEnum,
				  
				  "Dictionary", CMDSTRINGTYPE, &dictfile,
				  "d", CMDSTRINGTYPE, &dictfile,
				  
				  "DictionaryUpperBound", CMDINTTYPE, &dub,
				  "dub", CMDINTTYPE, &dub,
				  
				  "NgramSize", CMDSUBRANGETYPE, &size, 1 , MAX_NGRAM,
				  "n", CMDSUBRANGETYPE, &size, 1 , MAX_NGRAM,
				  				  
				  "Ngram", CMDSTRINGTYPE, &trainfile,
				  "TrainOn", CMDSTRINGTYPE, &trainfile,
				  "tr", CMDSTRINGTYPE, &trainfile,
				  
				  "oASR", CMDSTRINGTYPE, &ASRfile,
				  "oasr", CMDSTRINGTYPE, &ASRfile,
				  
				  "o", CMDSTRINGTYPE, &ARPAfile,
				  "oARPA", CMDSTRINGTYPE, &ARPAfile,
				  "oarpa", CMDSTRINGTYPE, &ARPAfile,
								
					"oBIN", CMDSTRINGTYPE, &BINfile,
					"obin", CMDSTRINGTYPE, &BINfile,
								
				  "TestOn", CMDSTRINGTYPE, &testfile, 
				  "te", CMDSTRINGTYPE, &testfile, 
				  
				  "AdaptOn", CMDSTRINGTYPE, &adaptfile, 
				  "ad", CMDSTRINGTYPE, &adaptfile, 
				  
				  "AdaptRate",CMDDOUBLETYPE , &adaptrate, 
				  "ar", CMDDOUBLETYPE, &adaptrate, 
				  
				  "AdaptLevel", CMDSUBRANGETYPE, &adaptlevel, 1 , MAX_NGRAM,
				  "al",CMDSUBRANGETYPE , &adaptlevel, 1, MAX_NGRAM,
				  
				  "AdaptOOV", CMDENUMTYPE, &adaptoov, BooleanEnum,
				  "ao", CMDENUMTYPE, &adaptoov, BooleanEnum,
				  
				  "LanguageModelType",CMDENUMTYPE, &lmtype, LmTypeEnum,
				  "lm",CMDENUMTYPE, &lmtype, LmTypeEnum,
				  
				  "Interactive",CMDENUMTYPE, &interactive, InteractiveModeEnum,
				  "i",CMDENUMTYPE, &interactive, InteractiveModeEnum,
				  
				  "Statistics",CMDSUBRANGETYPE, &statistics, 1 , 3,
				  "s",CMDSUBRANGETYPE, &statistics, 1 , 3,
				  
				  "PruneThresh",CMDSUBRANGETYPE, &prunefreq, 1 , 1000,
				  "p",CMDSUBRANGETYPE, &prunefreq, 1 , 1000,
				  
				  "PruneSingletons",CMDENUMTYPE, &prunesingletons, BooleanEnum,
				  "ps",CMDENUMTYPE, &prunesingletons, BooleanEnum,
				  
				  "PruneTopSingletons",CMDENUMTYPE, &prunetopsingletons, BooleanEnum,
				  "pts",CMDENUMTYPE, &prunetopsingletons, BooleanEnum,
				  
				  "ComputeLMSize",CMDENUMTYPE, &compsize, BooleanEnum,
				  "sz",CMDENUMTYPE, &compsize, BooleanEnum,
								
					"MaximumCachingLevel", CMDINTTYPE , &max_caching_level, 
					"mcl", CMDINTTYPE, &max_caching_level, 
								
					"MemoryMap", CMDENUMTYPE, &memmap, BooleanEnum,
					"memmap", CMDENUMTYPE, &memmap, BooleanEnum,
					"mm", CMDENUMTYPE, &memmap, BooleanEnum,
				  
				  "CheckProb",CMDENUMTYPE, &checkpr, BooleanEnum,
				  "cp",CMDENUMTYPE, &checkpr, BooleanEnum,
				  
				  "OutProb",CMDSTRINGTYPE, &outpr,
				  "op",CMDSTRINGTYPE, &outpr,
				  
				  "SubLMInfo", CMDSTRINGTYPE, &slminfo, 
				  "slmi", CMDSTRINGTYPE, &slminfo, 
				  
				  "SaveMixParam", CMDSTRINGTYPE, &omixpar, 
				  "smp", CMDSTRINGTYPE, &omixpar, 
				  
				  "LoadMixParam", CMDSTRINGTYPE, &imixpar, 
				  "lmp", CMDSTRINGTYPE, &imixpar, 
				  						
				  "SetOovRate", CMDDOUBLETYPE, &oovrate,
				  "or", CMDDOUBLETYPE, &oovrate,
				  
				  "Beta", CMDDOUBLETYPE, &beta,
				  "beta", CMDDOUBLETYPE, &beta,
				  
				  (char *)NULL
				  );
	
	GetParams(&argc, &argv, (char*) NULL);
	
	if (!trainfile || !lmtype)
    {
		cerr <<"Missing parameters\n";
		exit(1);
    };
	
	
	
	
	mdiadaptlm *lm=NULL;
	
	switch (lmtype){
			
		case SHIFT_BETA: 
			if (beta==-1 || (beta<1.0 && beta>0))
				lm=new shiftbeta(trainfile,size,prunefreq,beta,(backoff?SHIFTBETA_B:SHIFTBETA_I));
			else{
				cerr << "ShiftBeta: beta must be >0 and <1\n";
				exit(1);
			}
			break;
			
		case MOD_SHIFT_BETA: 
			if (size>1)      
				lm=new mshiftbeta(trainfile,size,prunefreq,(backoff?MSHIFTBETA_B:MSHIFTBETA_I));
			else{
				cerr << "Modified Shift Beta requires size > 1!\n";
				exit(1);
			}
			
			break;
			
		case SHIFT_ONE:
			lm=new shiftone(trainfile,size,prunefreq,(backoff?SIMPLE_B:SIMPLE_I));
			break;
			
		case LINEAR_WB:
			lm=new linearwb(trainfile,size,prunefreq,(backoff?MSHIFTBETA_B:MSHIFTBETA_I));
			break;
			
		case LINEAR_GT:
			cerr << "This LM is no more supported\n";
			break;
			
		case MIXTURE:
			lm=new mixture(trainfile,slminfo,size,prunefreq,imixpar,omixpar);
			break;
			
		default:
			cerr << "not implemented yet\n";
			return 1;
	};
	
	if (dub)      lm->dub(dub);
	lm->create_caches(max_caching_level);
	
	cerr << "eventually generate OOV code\n";
	lm->dict->genoovcode();
	
	if (oovrate) lm->dict->setoovrate(oovrate);
	
	
	lm->train();
	
	lm->prunesingletons(prunesingletons==YES);	
	lm->prunetopsingletons(prunetopsingletons==YES);
	
	if (prunetopsingletons==YES) //keep most specific 
		lm->prunesingletons(NO);
	
	if (adaptoov) lm->dict->incflag(1);
	
	if (adaptfile)
		lm->adapt(adaptfile,adaptlevel,adaptrate);
	
	if (adaptoov) lm->dict->incflag(0);    
	
	if (backoff) lm->compute_backoff();
	
	if (size>lm->maxlevel()){
		cerr << "lm size is too large\n";
		exit(1);
	}
	
	if (!size) size=lm->maxlevel();
	
	if (testfile){
		cerr << "TLM: test ...";
		lm->test(testfile,size,backoff,checkpr,outpr);
		
		if (adaptfile)
			((mdiadaptlm *)lm)->get_zetacache()->stat();
//		((mdiadaptlm *)lm)->cache->stat();
		
		//for (int s=1;s<=size;s++){
		//lm->test(*lm,s);
		//lm->test(test,s);
		//}
		cerr << "\n";
	};
	
	if (compsize) 
		cout << "LM size " << (int)lm->netsize() << "\n";
	
	if (interactive){
		
		ngram ng(lm->dict);
		int nsize=0;
		
		cout.setf(ios::scientific);
		
		switch (interactive){
				
			case NGRAM: 
				cout << "> ";
				while(cin >> ng){
					if (ng.wordp(size)){
						cout << ng << " p=" << (double)log(lm->prob(ng,size)) << "\n";
						ng.size=0;
						cout << "> ";
					}
				}
				break;
				
			case SEQUENCE:
			{
				char c;
				double p=0;
				cout << "> ";
				
				while(cin >> ng){
					nsize=ng.size<size?ng.size:size;
					p=log(lm->prob(ng,nsize));
					cout << ng << " p=" << p << "\n";
					
					while((c=cin.get())==' '){
						cout << c;
					}
					cin.putback(c);
					//cout << "-" << c << "-";
					if (c=='\n'){
						ng.size=0;
						cout << "> ";
						p=0;
					}
				}
			}
				
				break;
				
			case TURN: 
			{
				int n=0;
				double lp=0;
				double oov=0;
				
				while(cin >> ng){
					
					if (ng.size>0){
						nsize=ng.size<size?ng.size:size; 
						lp-=log(lm->prob(ng,nsize));
						n++;
						if (*ng.wordp(1) == lm->dict->oovcode())
							oov++;
					}
					else{
						if (n>0) cout << n << " " << lp/(log(2.0) * n) << " " << oov/n << "\n";
						n=0;
						lp=0;
						oov=0;
					}
				}
				
				break;
			}
				
			case  TEXT:
			{
				int order;	
				
				int n=0;
				double lp=0;
				double oov=0;
				
				while (!cin.eof()){
					cin >> order;
					if (order>size)
						cerr << "Warning: order > lm size\n";
					
					order=order>size?size:order;
					
					while (cin >> ng){
						if (ng.size>0){
							nsize=ng.size<order?ng.size:order; 
							lp-=log(lm->prob(ng,nsize));
							n++;
							if (*ng.wordp(1) == lm->dict->oovcode())
								oov++;
						}
						else{
							if (n>0) cout << n << " " << lp/(log(2.0)*n) << " " << oov/n << "\n";
							n=0;
							lp=0;
							oov=0;
							if (ng.isym>0) break;
						} 
					}
				}
			}      
				break;
				
			case ADAPT:
			{
				
				if (backoff){
					cerr << "This modality is not supported with backoff LMs\n";
					exit(1);
				}
				
				char afile[50],tfile[50];
				while (!cin.eof()){
					cin >> afile >> tfile;
					system("echo > .tlmlock");
					
					cerr << "interactive adaptation: " 
					<< afile << " " << tfile << "\n";
					
					if (adaptoov) lm->dict->incflag(1);
					lm->adapt(afile,adaptlevel,adaptrate);
					if (adaptoov) lm->dict->incflag(0);
					if (ASRfile) lm->saveASR(ASRfile,backoff,dictfile);
					if (ARPAfile) lm->saveARPA(ARPAfile,backoff,dictfile);
					if (BINfile) lm->saveBIN(BINfile,backoff,dictfile,memmap);
					lm->test(tfile,size,checkpr);
					cout.flush();
					system("rm .tlmlock");
				}
			}
				break;
		}
		
		exit(1);
	}
	
	if (ASRfile){
		cerr << "TLM: save lm (ASR)...";
		lm->saveASR(ASRfile,backoff,dictfile);
		cerr << "\n";
	}
	
	if (ARPAfile){
		cerr << "TLM: save lm (ARPA)...";
		lm->saveARPA(ARPAfile,backoff,dictfile);
		cerr << "\n";
	}

	if (BINfile){
		cerr << "TLM: save lm (binary)...";
		lm->saveBIN(BINfile,backoff,dictfile,memmap);
		cerr << "\n";
	}
	
	if (statistics){
		cerr << "TLM: lm stat ...";
		lm->lmstat(statistics);
		cerr << "\n";
	}

//	lm->cache_stat();
	
	cerr << "TLM: deleting lm ...";
	//delete lm;
	cerr << "\n";
	
	exit(0);
}



