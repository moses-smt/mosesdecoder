// $Id: lmtable.cpp 3686 2010-10-15 11:55:32Z bertoldi $

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
#include <stdio.h>
#include <cstdlib>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <set>
#include <cassert>
#include <limits>
#include "math.h"
#include "mempool.h"
#include "htable.h"
#include "ngramcache.h"
#include "dictionary.h"
#include "n_gram.h"
#include "lmtable.h"

#include "irstlm-util.h"

#define DEBUG 0

//special value for pruned iprobs
//#define NOPROB (int) -2
//#define NOPROB (float) -2
#define NOPROB ((float)-1.329227995784915872903807060280344576e36)

using namespace std;

inline void error(const char* message){
  std::cerr << message << "\n";
  //throw std::runtime_error(message);
}

void print(prob_and_state_t* pst, std::ostream& out){
  if (pst != NULL){
    out << "PST [";
    out << "logpr:" << pst->logpr;
    out << ",state:" << (void*) pst->state;
    out << ",statesize:" << pst->statesize;
    out << ",bow:" << pst->bow;
    out << ",bol:" << pst->bol;
    out << "]";
    out << std::endl;
  }else{
    out << "PST [NULL]" << std::endl;
  }
}

//instantiate an empty lm table
lmtable::lmtable(float nlf, float dlf){
  ngramcache_load_factor = nlf;	
  dictionary_load_factor = dlf;	
  configure(1,false);
	
  dict=new dictionary((char *)NULL,1000000,dictionary_load_factor);
	
  memset(table, 0, sizeof(table));
  memset(tableGaps, 0, sizeof(tableGaps));
  memset(cursize, 0, sizeof(cursize));
  memset(tbltype, 0, sizeof(tbltype));
  memset(maxsize, 0, sizeof(maxsize));
  memset(info, 0, sizeof(info));
  memset(NumCenters, 0, sizeof(NumCenters));

  max_cache_lev=0;
  for (int i=0;i<LMTMAXLEV+1;i++) lmtcache[i]=NULL;
  prob_and_state_cache=NULL;
	
#ifdef TRACE_CACHELM
  //cacheout=new std::fstream(get_temp_folder()++"tracecache",std::ios::out);
  cacheout=new std::fstream("/tmp/tracecache",std::ios::out);
  sentence_id=0;
#endif
 
  memmap=0;
  
  isPruned=false;
  isInverted=false;
	
  //statistics
  for (int i=0;i<=LMTMAXLEV+1;i++) totget[i]=totbsearch[i]=0;
	
  logOOVpenalty=0.0; //penalty for OOV words (default 0)
	
  // by default, it is a standard LM, i.e. queried for score
  setOrderQuery(false);
};

lmtable::~lmtable(){
	delete_caches();
	
#ifdef TRACE_CACHELM
	cacheout->close();
	delete cacheout;
#endif	
	
	for (int l=1;l<=maxlev;l++){
		if (table[l]){
			if (memmap > 0 && l >= memmap)
				Munmap(table[l]-tableGaps[l],cursize[l]*nodesize(tbltype[l])+tableGaps[l],0);
			else
				delete [] table[l];
				}
				if (isQtable){
					if (Pcenters[l]) delete [] Pcenters[l];
					if (l<maxlev)
						if (Bcenters[l]) delete [] Bcenters[l];
				}
	}
	
	delete dict;
};

void lmtable::init_prob_and_state_cache(){
#ifdef PS_CACHE_ENABLE
  assert(prob_and_state_cache==NULL);
  prob_and_state_cache=new NGRAMCACHE_t(maxlev,sizeof(prob_and_state_t),400000,ngramcache_load_factor); // initial number of entries is 2^18
  std::cerr << "creating cache for storing prob, state and statesize of ngrams" << std:: endl;
#endif
}

void lmtable::init_lmtcaches(int uptolev){
  max_cache_lev=uptolev;
#ifdef LMT_CACHE_ENABLE	
  for (int i=2;i<=max_cache_lev;i++){
    assert(lmtcache[i]==NULL);
    lmtcache[i]=new NGRAMCACHE_t(i,sizeof(char*),200000,ngramcache_load_factor); // initial number of entries is 2^18
    std::cerr << "creating cache for storing pointers to the LM for ngram of size " << i << std:: endl;
  }
#endif
}

void lmtable::init_caches(int uptolev){
        init_prob_and_state_cache();
	init_lmtcaches(uptolev);
}

void lmtable::delete_prob_and_state_cache(){
#ifdef PS_CACHE_ENABLE
  if (prob_and_state_cache) delete prob_and_state_cache;
  prob_and_state_cache=NULL;
  std::cerr << "deleting cache for storing prob, state and statesize of ngrams" << std:: endl;
#endif
}

void lmtable::delete_lmtcaches(){
#ifdef LMT_CACHE_ENABLE   
  for (int i=2;i<=max_cache_lev;i++){
    if (lmtcache[i]) delete lmtcache[i];
    lmtcache[i]=NULL;
    std::cerr << "deleting cache for storing pointers to the LM for ngram of size " << i << std:: endl;
  }
#endif
}

void lmtable::delete_caches(){
        delete_prob_and_state_cache();
        delete_lmtcaches();
}


void lmtable::used_prob_and_state_cache(){
#ifdef PS_CACHE_ENABLE
  std::cerr << "prob_and_state_cache() ";
  if (prob_and_state_cache) prob_and_state_cache->used();
#endif
}

void lmtable::used_lmtcaches(){
#ifdef LMT_CACHE_ENABLE   
  for (int i=2;i<=max_cache_lev;i++){
    std::cerr << "lmtcaches with order " << i << " ";
    if (lmtcache[i]) lmtcache[i]->used();
  }
#endif
}

void lmtable::used_caches(){
        used_prob_and_state_cache();
        used_lmtcaches();
}


void lmtable::check_prob_and_state_cache_levels(){
#ifdef PS_CACHE_ENABLE
  if (prob_and_state_cache && prob_and_state_cache->isfull())
         prob_and_state_cache->reset(prob_and_state_cache->cursize());
#endif
}

void lmtable::check_lmtcaches_levels(){
#ifdef LMT_CACHE_ENABLE	
  for (int i=2;i<=max_cache_lev;i++)
    if (lmtcache[i]->isfull()) lmtcache[i]->reset(lmtcache[i]->cursize());
#endif
}

void lmtable::check_caches_levels(){
	check_prob_and_state_cache_levels();
	check_lmtcaches_levels();
}

void lmtable::reset_prob_and_state_cache(){
#ifdef PS_CACHE_ENABLE  
  if (prob_and_state_cache)
        prob_and_state_cache->reset(MAX(prob_and_state_cache->cursize(),prob_and_state_cache->maxsize()));
#endif
}

void lmtable::reset_lmtcaches(){
#ifdef LMT_CACHE_ENABLE   
  for (int i=2;i<=max_cache_lev;i++)
    lmtcache[i]->reset(MAX(lmtcache[i]->cursize(),lmtcache[i]->maxsize()));
#endif
}

void lmtable::reset_caches(){
  reset_prob_and_state_cache();
  reset_lmtcaches();
}

bool lmtable::are_prob_and_state_cache_active(){
#ifdef PS_CACHE_ENABLE
        return prob_and_state_cache!=NULL;
#else
        return false;
#endif
}

bool lmtable::are_lmtcaches_active(){
#ifdef LMT_CACHE_ENABLE
	if (max_cache_lev < 2)
		return false;
        for (int i=2;i<=max_cache_lev;i++)
		if (lmtcache[i]==NULL) return false;
	return true;
#else
	return false;
#endif
}

bool lmtable::are_caches_active(){
        return (are_prob_and_state_cache_active() && are_lmtcaches_active());
}

void lmtable::configure(int n,bool quantized){
  maxlev=n;
  if (n==1)
    tbltype[1]=(quantized?QLEAF:LEAF);
  else{
    for (int i=1;i<n;i++) tbltype[i]=(quantized?QINTERNAL:INTERNAL);
    tbltype[n]=(quantized?QLEAF:LEAF);
  }
}



void lmtable::load(istream& inp,const char* filename,const char* outfilename,int keep_on_disk, OUTFILE_TYPE /* unused parameter: outtype */){
	
#ifdef WIN32
  if (keep_on_disk>0){
    std::cerr << "lmtable::load memory mapping not yet available under WIN32\n";
    keep_on_disk = 0;
  }
#endif
	
  //give a look at the header to select loading method
  char header[MAX_LINE];
  inp >> header; cerr << header << "\n";
	
  if (strncmp(header,"Qblmt",5)==0 || strncmp(header,"blmt",4)==0){
    loadbin(inp,header,filename,keep_on_disk);
  }
  else{ //input is in textual form
    
	  if (keep_on_disk && outfilename==NULL) {
      cerr << "Load Error: inconsistent setting. Passed input file: textual. Memory map: yes. Outfilename: not specified.\n";
      exit(0);
    }
		
    loadtxt(inp,header,outfilename,keep_on_disk);
  }
	
  cerr << "OOV code is " << lmtable::getDict()->oovcode() << "\n";
}


//load language model on demand through a word-list file

int lmtable::reload(std::set<string> words){
	
	
	//build dictionary
	dictionary dict(NULL,(int)words.size()); dict.incflag(1);
	
	std::set<string>::iterator w;
	for (w = words.begin(); w != words.end(); ++w)
		dict.encode((*w).c_str());
		
	return 1;	
}

int parseWords(char *sentence, const char **words, int max)
{
  char *word;
  int i = 0;
	
  const char *const wordSeparators = " \t\r\n";
	
  for (word = strtok(sentence, wordSeparators);
       i < max && word != 0;
       i++, word = strtok(0, wordSeparators))
	{
		words[i] = word;
	}
	
  if (i < max){words[i] = 0;}
	
  return i;
}



//Load a LM as a text file. LM could have been generated either with the
//IRST LM toolkit or with the SRILM Toolkit. In the latter we are not
//sure that n-grams are lexically ordered (according to the 1-grams).
//However, we make the following assumption:
//"all successors of any prefix are sorted and written in contiguous lines!"
//This method also loads files processed with the quantization
//tool: qlm

int parseline(istream& inp, int Order,ngram& ng,float& prob,float& bow){
	
  const char* words[1+ LMTMAXLEV + 1 + 1];
  int howmany;
  char line[MAX_LINE];
	
  inp.getline(line,MAX_LINE);
  if (strlen(line)==MAX_LINE-1){
    cerr << "parseline: input line exceed MAXLINE ("
		<< MAX_LINE << ") chars " << line << "\n";
    exit(1);
  }
	
  howmany = parseWords(line, words, Order + 3);
	
  if (!(howmany == (Order+ 1) || howmany == (Order + 2)))
		assert(howmany == (Order+ 1) || howmany == (Order + 2));
	
  //read words
  ng.size=0;
  for (int i=1;i<=Order;i++)
    ng.pushw(strcmp(words[i],"<unk>")?words[i]:ng.dict->OOV());
	
  //read logprob/code and logbow/code
  assert(sscanf(words[0],"%f",&prob));
  if (howmany==(Order+2))
    assert(sscanf(words[Order+1],"%f",&bow));
  else
    bow=0.0; //this is log10prob=0 for implicit backoff
	
  /**
		if (Order>1) {
			cout << prob << "\n";
			for (int i=1;i<=Order;i++)
				cout <<  words[i] << " ";
			cout << "\n" << bow  << "\n\n";
		}
	 **/
  return 1;
}


void lmtable::loadcenters(istream& inp,int Order){
  char line[MAX_LINE];
	
  //first read the coodebook
  cerr << Order << " read code book ";
  inp >> NumCenters[Order];
  Pcenters[Order]=new float[NumCenters[Order]];
  Bcenters[Order]=(Order<maxlev?new float[NumCenters[Order]]:NULL);
	
  for (int c=0;c<NumCenters[Order];c++){
    inp >> Pcenters[Order][c];
    if (Order<maxlev) inp >> Bcenters[Order][c];
  };
  //empty the last line
  inp.getline((char*)line,MAX_LINE);
}

void lmtable::loadtxt(istream& inp,const char* header,const char* outfilename,int mmap){
  if (mmap>0)
    loadtxtmmap(inp,header,outfilename);
  else {
    loadtxt(inp,header);
    lmtable::getDict()->genoovcode();
  }
}

void lmtable::loadtxtmmap(istream& inp,const char* header,const char* outfilename){
	
	char nameNgrams[BUFSIZ];
	char nameHeader[BUFSIZ];
	
	FILE *fd = NULL;
	table_pos_t filesize=0;
	
	int Order,n;
	
	int maxlevel_h;
	//char *SepString = " \t\n"; unused
	
	//open input stream and prepare an input string
	char line[MAX_LINE];
	
	//prepare word dictionary
	//dict=(dictionary*) new dictionary(NULL,1000000,NULL,NULL);
	lmtable::getDict()->incflag(1);
	
	//put here ngrams, log10 probabilities or their codes
	ngram ng(lmtable::getDict());
	ngram ing(lmtable::getDict());
	
	float pb,bow;
	
	//check the header to decide if the LM is quantized or not
	isQtable=(strncmp(header,"qARPA",5)==0?true:false);
	
	//check the header to decide if the LM table is incomplete
	isItable=(strncmp(header,"iARPA",5)==0?true:false);
	
	if (isQtable){
		//check if header contains other infos
		inp >> line;
		if (!(maxlevel_h=atoi(line))){
			cerr << "loadtxt with mmap requires new qARPA header. Please regenerate the file.\n";
			exit(1);
		}
		
		for (n=1;n<=maxlevel_h;n++){
			inp >> line;
			if (!(NumCenters[n]=atoi(line))){
				cerr << "loadtxt with mmap requires new qARPA header. Please regenerate the file.\n";
				exit(0);
			}
		}
	}
	
	//we will configure the table later we we know the maxlev;
	bool yetconfigured=false;
	
	cerr << "loadtxtmmap()\n";
	
	// READ ARPA Header
	
	while (inp.getline(line,MAX_LINE)){
		
		if (strlen(line)==MAX_LINE-1){
			cerr << "lmtable::loadtxtmmap: input line exceed MAXLINE ("
			<< MAX_LINE << ") chars " << line << "\n";
			exit(1);
		}
		
		bool backslash = (line[0] == '\\');
		
		if (sscanf(line, "ngram %d=%d", &Order, &n) == 2) {
			maxsize[Order] = n; maxlev=Order; //upadte Order
			cerr << "size[" << Order << "]=" << maxsize[Order] << "\n";
		}
		
		if (backslash && sscanf(line, "\\%d-grams", &Order) == 1) {
			
			//at this point we are sure about the size of the LM
			if (!yetconfigured){
				configure(maxlev,isQtable);
				yetconfigured=true;
				
				//opening output file
				strcpy(nameNgrams,outfilename);
				strcat(nameNgrams, "-ngrams");
				
				cerr << "saving ngrams probs in " << nameNgrams << "\n";
				
				fd = fopen(nameNgrams, "w+");
				
				// compute the size of file (only for tables and - possibly - centroids; no header nor dictionary)
				for (int l=1;l<=maxlev;l++){
					if (l<maxlev)
						filesize +=  (table_pos_t) maxsize[l] * nodesize(tbltype[l]) + 2 * NumCenters[l] * sizeof(float);
					else
						filesize +=  (table_pos_t) maxsize[l] * nodesize(tbltype[l]) + NumCenters[l] * sizeof(float);
				}
				
				cerr << "global filesize = " << filesize << "\n";
				// set the file to the proper size:
				ftruncate(fileno(fd),filesize);
				table[0]=(char *)(MMap(fileno(fd),PROT_READ|PROT_WRITE,0,filesize,&tableGaps[0]));
				
				//allocate space for tables into the file through mmap:
				
				if (maxlev>1)
					table[1]=table[0] + (table_pos_t) (2 * NumCenters[1] * sizeof(float));
				else
					table[1]=table[0] + (table_pos_t) (NumCenters[1] * sizeof(float));
				
				for (int l=2;l<=maxlev;l++)
					if (l<maxlev)
						table[l]=(char *)(table[l-1] + (table_pos_t) maxsize[l-1]*nodesize(tbltype[l-1]) +
															2 * NumCenters[l] * sizeof(float));
					else
						table[l]=(char *)(table[l-1] + (table_pos_t) maxsize[l-1]*nodesize(tbltype[l-1]) +
															NumCenters[l] * sizeof(float));
				
				for (int l=2;l<=maxlev;l++){
					cerr << "table[" << l << "]-table[" << l-1 << "]="
					<< (table_pos_t) table[l]-(table_pos_t) table[l-1] << " (nodesize=" << nodesize(tbltype[l-1]) << ")\n";
				}
			}
			
			cerr << Order << "-grams: reading ";
			if (isQtable) {
				loadcenters(inp,Order);
				// writing centroids on disk
				if (Order<maxlev){
					memcpy(table[Order] - 2 * NumCenters[Order] * sizeof(float),
								 Pcenters[Order],
								 NumCenters[Order] * sizeof(float));
					memcpy(table[Order] - NumCenters[Order] * sizeof(float),
								 Bcenters[Order],
								 NumCenters[Order] * sizeof(float));
				} else
					memcpy(table[Order] - NumCenters[Order] * sizeof(float),
								 Pcenters[Order],
								 NumCenters[Order] * sizeof(float));
			}
			
			//allocate support vector to manage badly ordered n-grams
			if (maxlev>1 && Order<maxlev) {
				startpos[Order]=new table_entry_pos_t[maxsize[Order]];
				for (table_entry_pos_t c=0;c<maxsize[Order];c++) startpos[Order][c]=BOUND_EMPTY1;
			}
			//prepare to read the n-grams entries
			cerr << maxsize[Order] << " entries\n";
			
			//WE ASSUME A WELL STRUCTURED FILE!!!
			for (table_entry_pos_t c=0;c<maxsize[Order];c++){
				
				if (parseline(inp,Order,ng,pb,bow)){
					//if table is in incomplete ARPA format pb is just the
					//discounted frequency, so we need to add bow * Pr(n-1 gram)
					
					// if table is inverted then revert n-gram
					if (isInverted & Order>1){
						ing.invert(ng);
						ng=ing;
					}
					
					if (isItable && Order>1){
						//get bow of lower of context
						get(ng,ng.size,ng.size-1);
						float rbow=0.0;
						if (ng.lev==ng.size-1){ //found context
							rbow=ng.bow;
							//  int ibow=ng.bow; rbow=*((float *)&ibow);
						}
						
						int tmp=maxlev;
						maxlev=Order-1;
						pb= log(exp((double)pb * M_LN10) +  exp(((double)rbow + lprob(ng)) * M_LN10))/M_LN10;
						maxlev=tmp;
					}
					
					if (isQtable) add(ng, (qfloat_t)pb, (qfloat_t)bow);
					else add(ng, pb, bow);
					
				}
			}
			// To avoid huge memory write concentrated at the end of the program
			msync(table[0],filesize,MS_SYNC);
			
			// now we can fix table at level Order -1
			// (not required if the input LM is in lexicographical order)
			if (maxlev>1 && Order>1){
				checkbounds(Order-1);
				delete startpos[Order-1];
			}
		}
	}
	
	cerr << "closing output file: " << nameNgrams << "\n";
	for (int i=1;i<=maxlev;i++)
		if (maxsize[i] != cursize[i]) {
			for (int l=1;l<=maxlev;l++)
				cerr << "Level " << l << ": starting ngrams=" << maxsize[l] << " - actual stored ngrams=" << cursize[l] << "\n";
			break;
		}
			
			Munmap(table[0],filesize,MS_SYNC);
	for (int l=1;l<=maxlev;l++)
		table[l]=0; // to avoid wrong free in ~lmtable()
	cerr << "running fclose...\n";
	fclose(fd);
	cerr << "done\n";
	
	lmtable::getDict()->incflag(0);
	lmtable::getDict()->genoovcode();
	
	// saving header + dictionary
	
	strcpy(nameHeader,outfilename);
	strcat(nameHeader, "-header");
	cerr << "saving header+dictionary in " << nameHeader << "\n";
	fstream out(nameHeader,ios::out);
	
	// print header
	if (isQtable){
		out << "Qblmt" << (isInverted?"I ":" ") << maxlev;
		for (int i=1;i<=maxlev;i++) out << " " << maxsize[i];  // not cursize[i] because the file was already allocated
		out << "\nNumCenters";
		for (int i=1;i<=maxlev;i++)  out << " " << NumCenters[i];
		out << "\n";
		
	}else{
		out << "blmt" << (isInverted?"I ":" ") << maxlev;
		for (int i=1;i<=maxlev;i++) out << " " << maxsize[i];  // not cursize[i] because the file was already allocated
		out << "\n";
	}
	
	lmtable::getDict()->save(out);
	
	out.close();
	cerr << "done\n";
	
	// cat header+dictionary and n-grams files:
	
	char cmd[BUFSIZ];
	sprintf(cmd,"cat %s >> %s", nameNgrams, nameHeader);
	cerr << "run cmd <" << cmd << ">\n";
	system(cmd);
	
	sprintf(cmd,"mv %s %s", nameHeader, outfilename);
	cerr << "run cmd <" << cmd << ">\n";
	system(cmd);
	
	sprintf(cmd,"rm %s", nameNgrams);
	cerr << "run cmd <" << cmd << ">\n";
	system(cmd);
	
	//no more operations are available, the file must be saved!
	exit(0);
	return;
}

void lmtable::loadtxt(istream& inp,const char* header){
	
	
	//open input stream and prepare an input string
	char line[MAX_LINE];
	
	//prepare word dictionary
	//dict=(dictionary*) new di
	dictionary(NULL,1000000);
	lmtable::getDict()->incflag(1);
	
	//put here ngrams, log10 probabilities or their codes
	ngram ng(lmtable::getDict());	
	ngram ing(lmtable::getDict()); //support n-gram 
	
	float prob,bow;
	
	//check the header to decide if the LM is quantized or not
	isQtable=(strncmp(header,"qARPA",5)==0?true:false);
	
	//check the header to decide if the LM table is incomplete
	isItable=(strncmp(header,"iARPA",5)==0?true:false);
	
	//we will configure the table later we we know the maxlev;
	bool yetconfigured=false;
	
	cerr << "loadtxt()\n";
	
	// READ ARPA Header
	int Order,n;
	
	while (inp.getline(line,MAX_LINE)){
		
		if (strlen(line)==MAX_LINE-1){
			cerr << "lmtable::loadtxt: input line exceed MAXLINE ("
			<< MAX_LINE << ") chars " << line << "\n";
			exit(1);
		}
		
		bool backslash = (line[0] == '\\');
		
		if (sscanf(line, "ngram %d=%d", &Order, &n) == 2) {
			maxsize[Order] = n; maxlev=Order; //upadte Order
			
		}
		
		if (backslash && sscanf(line, "\\%d-grams", &Order) == 1) {
			
			//at this point we are sure about the size of the LM
			if (!yetconfigured){
				configure(maxlev,isQtable);yetconfigured=true;
				//allocate space for loading the table of this level
				for (int i=1;i<=maxlev;i++)
					table[i] = new char[(table_pos_t) maxsize[i] * nodesize(tbltype[i])];
			}
			
			cerr << Order << "-grams: reading ";
			
			if (isQtable) loadcenters(inp,Order);
			
			//allocate support vector to manage badly ordered n-grams
			if (maxlev>1 && Order<maxlev) {
				startpos[Order]=new table_entry_pos_t[maxsize[Order]];
				for (table_entry_pos_t c=0;c<maxsize[Order];c++){
					startpos[Order][c]=BOUND_EMPTY1;
				}
			}
			
			//prepare to read the n-grams entries
			cerr << maxsize[Order] << " entries\n";
			
			//WE ASSUME A WELL STRUCTURED FILE!!!
			
			for (table_entry_pos_t c=0;c<maxsize[Order];c++){
				
				if (parseline(inp,Order,ng,prob,bow)){
					
					// if table is inverted then revert n-gram
					if (isInverted & Order>1){
						ing.invert(ng);
						ng=ing;
					}
					
					//if table is in incomplete ARPA format prob is just the
					//discounted frequency, so we need to add bow * Pr(n-1 gram)
					
					//cerr << "ng: " << ng << " prob: " << prob << " bow: " << bow << "\n";
					
					if (isItable && Order>1) {
						//get bow of lower context
						get(ng,ng.size,ng.size-1);
						float rbow=0.0;
						if (ng.lev==ng.size-1){ //found context
							rbow=ng.bow;
							//              int ibow=ng.bow; rbow=*((float *)&ibow);
						}
						
						int tmp=maxlev;
						maxlev=Order-1;
						//            cerr << ng << "rbow: " << rbow << "prob: " << prob << "low-prob: " << lprob(ng) << "\n";
						prob= log(exp((double)prob * M_LN10) +  exp(((double)rbow + lprob(ng)) * M_LN10))/M_LN10;
						//            cerr << "new prob: " << prob << "\n";
						
						maxlev=tmp;
					}
					
					if (isQtable) add(ng, (qfloat_t)prob, (qfloat_t)bow);
					else add(ng, prob, bow);
					
					/*
					 add(ng,
							 (int)(isQtable?prob:*((int *)&prob)),
							 (int)(isQtable?bow:*((int *)&bow)));
					 */
				}
			}
			// now we can fix table at level Order -1
			if (maxlev>1 && Order>1) checkbounds(Order-1);
		}
	}
	
	lmtable::getDict()->incflag(0);
	cerr << "done\n";
	
}

void lmtable::expand_level(int level, table_entry_pos_t size, const char* outfilename, int mmap){
	cerr << "expanding level: " << level << " with " << size << " entries ...\n";
  if (mmap>0)
    expand_level_mmap(level, size, outfilename);
  else {
    expand_level_nommap(level, size);
  }
}

void lmtable::expand_level_mmap(int level, table_entry_pos_t size, const char* outfilename){
	maxsize[level]=size;
	
	//getting the level-dependent filename
	char nameNgrams[BUFSIZ];
	sprintf(nameNgrams,"%s-%dgrams",outfilename,level);
	
	cerr << level << "-grams: creating level of size " << maxsize[level] << " in memory map on "<< nameNgrams << std::endl;
	
	//opening output file
	FILE *fd = NULL;
	fd = fopen(nameNgrams, "w+");
	if (fd == NULL) {
		perror("Error opening file for writing");
		exit(EXIT_FAILURE);
	}		
	table_pos_t filesize=(table_pos_t) maxsize[level] * nodesize(tbltype[level]);
	// set the file to the proper size:
	ftruncate(fileno(fd),filesize);
			
		/* Now the file is ready to be mmapped.
		*/
	table[level]=(char *)(MMap(fileno(fd),PROT_READ|PROT_WRITE,0,filesize,&tableGaps[level]));
	if (table[level] == MAP_FAILED) {
		fclose(fd);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}
	
//	memset(table[level],0,filesize);
		
	if (maxlev>1 && level<maxlev) {
				startpos[level]=new table_entry_pos_t[maxsize[level]];
				for (table_entry_pos_t c=0;c<maxsize[level];c++){
					startpos[level][c]=BOUND_EMPTY1;
				}
	}
}

void lmtable::expand_level_nommap(int level, table_entry_pos_t size){
	maxsize[level]=size;
	cerr << level << "-grams: creating level of size " << maxsize[level] << std::endl;
	table[level] = new char[(table_pos_t) maxsize[level] * nodesize(tbltype[level])];
	if (maxlev>1 && level<maxlev) {
				startpos[level]=new table_entry_pos_t[maxsize[level]];
				for (table_entry_pos_t c=0;c<maxsize[level];c++){
					startpos[level][c]=BOUND_EMPTY1;
				}
	}
}

void lmtable::printTable(int level) {
  char*  tbl=table[level];
  LMT_TYPE ndt=tbltype[level];
  int ndsz=nodesize(ndt);
  table_entry_pos_t printEntryN=1000;
  if (cursize[level]>0)
    printEntryN=(printEntryN<cursize[level])?printEntryN:cursize[level];
	
  cout << "level = " << level << "\n";
	
	//TOCHECK: Nicola, 18 dicembre 2009
  float p;
  for (table_entry_pos_t c=0;c<printEntryN;c++){
    p=prob(tbl,ndt);
    cout << p << " " << word(tbl) << "\n";
    //cout << *(float *)&p << " " << word(tbl) << "\n";
    tbl+=ndsz;
  }
  return;
}

//Checkbound with sorting of n-gram table on disk

void lmtable::checkbounds(int level){
	
	char*  tbl=table[level];
	char*  succtbl=table[level+1];
	
	LMT_TYPE ndt=tbltype[level], succndt=tbltype[level+1];
	int ndsz=nodesize(ndt), succndsz=nodesize(succndt);
	
	//re-order table at level+1 on disk
	//generate random filename to avoid collisions
	ofstream out;string filePath;
	createtempfile(out,filePath,ios::out|ios::binary);
	
	table_entry_pos_t start,end,newstart;
	
	//re-order table at level l+1
	newstart=0;
//	cerr << "BOUND_EMPTY1:" << BOUND_EMPTY1 << " BOUND_EMPTY2:" << BOUND_EMPTY2 << std::endl;
	for (table_entry_pos_t c=0;c<cursize[level];c++){
		start=startpos[level][c]; end=bound(tbl+ (table_pos_t) c*ndsz,ndt);
		//    start=startpos[level][c]; end=bound(tbl+c*ndsz,ndt);
		
		//is start==BOUND_EMPTY1 there are no successors for this entry and end==BOUND_EMPTY2
		if (start==BOUND_EMPTY1) end=BOUND_EMPTY2;
		if (end==BOUND_EMPTY2) end=start;
		
		assert(start<=end);
		assert(newstart+(end-start)<=cursize[level+1]);
		assert(end == BOUND_EMPTY1 || end<=cursize[level+1]);
		
		
		if (start<end){
			out.write((char*)(succtbl + (table_pos_t) start * succndsz),(table_pos_t) (end-start) * succndsz);
			if (!out.good()){
				std::cerr << " Something went wrong while writing temporary file " << filePath
				<< " Maybe there is not enough space on this filesystem\n";
				
				out.close();
				removefile(filePath);
			}
		}
		
		bound(tbl+(table_pos_t) c*ndsz,ndt,newstart+(end-start));
		newstart+=(end-start);
	}
	
	out.close();
	
	fstream inp(filePath.c_str(),ios::in|ios::binary);
	
	inp.read(succtbl,(table_pos_t) cursize[level+1]*succndsz);
	//  inp.read(succtbl,cursize[level+1]*succndsz);
	inp.close();
	
	
}

//Add method inserts n-grams in the table structure. It is ONLY used during
//loading of LMs in text format. It searches for the prefix, then it adds the
//suffix to the last level and updates the start-end positions.

//int lmtable::add(ngram& ng,int iprob,int ibow){
template<typename TA, typename TB> 
int lmtable::add(ngram& ng, TA iprob,TB ibow){
	
  char *found;
	LMT_TYPE ndt=tbltype[1]; //default initialization
	int ndsz=nodesize(ndt); //default initialization
	static int no_more_msg = 0;
	
	
//	cerr << "add(): ng.size: " << ng.size << " ng: |" << ng << "| cursize[ng.size]: " << cursize[ng.size] << " maxsize[ng.size]: " << maxsize[ng.size];
  if (ng.size>1){
		
    // find the prefix starting from the first level
    table_entry_pos_t start=0, end=cursize[1];
		
	  
    for (int l=1;l<ng.size;l++){

      ndt=tbltype[l]; ndsz=nodesize(ndt);
			
      if (search(l,start,(end-start),ndsz,
                 ng.wordp(ng.size-l+1),LMT_FIND, &found)){
				
        //update start-end positions for next step
        if (l< (ng.size-1)){
          //set start position
          if (found==table[l]) start=0; //first pos in table
          else start=bound(found - ndsz,ndt); //end of previous entry
					
          //set end position
          end=bound(found,ndt);
        }
      }
      else {
				if (!no_more_msg)
					cerr << "warning: missing back-off (at level " << l << ") for ngram " << ng << " (and possibly for others)\n";
				
				no_more_msg++;
				if (!(no_more_msg % 5000000))
					cerr << "!";
				
				return 0;
      }
    }
		
    // update book keeping information about level ng-size -1.
    // if this is the first successor update start position
    table_entry_pos_t position=(table_entry_pos_t) (((table_pos_t) found-(table_pos_t) table[ng.size-1])/ndsz);
    //table_entry_pos_t position=((table_entry_pos_t)((table_pos_t) found-(table_pos_t) table[ng.size-1])/ndsz);
		
    if (startpos[ng.size-1][position]==BOUND_EMPTY1)
      startpos[ng.size-1][position]=cursize[ng.size];

    //always update ending position
    bound(found,ndt,cursize[ng.size]+1);
  }
	
  // just add at the end of table[ng.size]
	
  assert(cursize[ng.size]< maxsize[ng.size]); // is there enough space?
  ndt=tbltype[ng.size];ndsz=nodesize(ndt);
	
  found=table[ng.size] + ((table_pos_t) cursize[ng.size] * ndsz);
  word(found,*ng.wordp(1));
  prob(found,ndt,iprob);
  if (ng.size<maxlev){
		bow(found,ndt,ibow);
		bound(found,ndt,BOUND_EMPTY2);
	}

//	cerr << " found: " << (void*) found << " table[ng.size]: " << (void*) table[ng.size] << "\n";
  
	cursize[ng.size]++;
	
  if (!(cursize[ng.size]%5000000))
    cerr << ".";
	
  return 1;
	
}


void *lmtable::search(int lev,
                      table_entry_pos_t offs,
                      table_entry_pos_t n,
                      int sz,
                      int *ngp,
                      LMT_ACTION action,
                      char **found){

  /***
	if (n >=2)
	cout << "searching entry for codeword: " << ngp[0] << "...";
  ***/
	
  //assume 1-grams is a 1-1 map of the vocabulary
  //CHECK: explicit cast of n into float because table_pos_t could be unsigned and larger than MAXINT
  if (lev==1) return *found=(*ngp < (float) n ? table[1] + (table_pos_t)*ngp * sz:NULL);
	
	
  //prepare table to be searched with mybserach
  char* tb;
  tb=table[lev] + (table_pos_t) offs * sz;
  //prepare search pattern
  char w[LMTCODESIZE];putmem(w,ngp[0],0,LMTCODESIZE);
	
  table_entry_pos_t idx=0; // index returned by mybsearch
  *found=NULL;	//initialize output variable
	
  totbsearch[lev]++;
  switch(action){
		case LMT_FIND:
			//    if (!tb || !mybsearch(tb,n,sz,(unsigned char *)w,&idx)) return NULL;

			if (!tb || !mybsearch(tb,n,sz,w,&idx)){
			 return NULL;
			}
			else{
				//      return *found=tb + (idx * sz);
				return *found=tb + ((table_pos_t)idx * sz);
			}
		default:
			error((char*)"lmtable::search: this option is available");
  };
  return NULL;
}


//int lmtable::mybsearch(char *ar, table_pos_t n, int size, unsigned char *key, table_pos_t *idx)
int lmtable::mybsearch(char *ar, table_entry_pos_t n, int size, char *key, table_entry_pos_t *idx)
{
  register table_entry_pos_t low, high;
  register char *p;
  register int result=0;
	/*
	 register unsigned char *p;
	 register table_pos_t result=0;
	 register int i;
	 */
	
  /* return idx with the first position equal or greater than key */
	
  /*   Warning("start bsearch \n"); */
	
  low = 0;high = n; *idx=0;
  while (low < high)
	{
		
		*idx = (low + high) / 2;
		
		p = (char *) (ar + ((table_pos_t)*idx * size));
		//      p = (unsigned char *) (ar + (*idx * size));
		
		//comparison
		/*
		 for (i=(LMTCODESIZE-1);i>=0;i--){
			 result=key[i]-p[i];
			 if (result) break;
		 }
		 */
		
		result=word(key)-word(p);
		
		if (result < 0)
			high = *idx;
		else if (result > 0)
			low = *idx + 1;
		else
			return 1;
	}
	
  *idx=low;
	
  return 0;
	
}


// generates a LM copy for a smaller dictionary

lmtable* lmtable::cpsublm(dictionary* subdict,bool keepunigr){
	
	//keepunigr=false;		
	//create new lmtable that inherits all features of this lmtable
	
	lmtable* slmt=new lmtable(ngramcache_load_factor,dictionary_load_factor);		
	slmt->configure(maxlev,isQtable);
	slmt->dict=new dictionary((keepunigr?dict:subdict),false);
	std::cerr << "subdict size: " << slmt->dict->size() << "\n";
	
	if (isQtable){
		for (int i=1;i<=maxlev;i++)  {
			slmt->NumCenters[i]=NumCenters[i];
			slmt->Pcenters[i]=new float [NumCenters[i]];
			memcpy(slmt->Pcenters[i],Pcenters[i],NumCenters[i] * sizeof(float));
			slmt->Bcenters[i]=new float [NumCenters[i]];
			memcpy(slmt->Bcenters[i],Bcenters[i],NumCenters[i] * sizeof(float));		
		}
	}
	
	//mange dictionary information
	
	//generate OOV codes and build dictionary lookup table 
	dict->genoovcode(); slmt->dict->genoovcode(); subdict->genoovcode();
	std::cerr << "subdict size: " << slmt->dict->size() << "\n";
	int* lookup;lookup=new int [dict->size()];
	for (int c=0;c<dict->size();c++){
		lookup[c]=subdict->encode(dict->decode(c));
		if (c != dict->oovcode() && lookup[c] == subdict->oovcode())
			lookup[c]=-1; // words of this->dict that are not in slmt->dict
	}
	
	//variables useful to navigate in the lmtable structure
	LMT_TYPE ndt,pndt; int ndsz,pndsz; 
	char *entry, *newentry; 
	table_entry_pos_t start, end, origin;
	
	for (int l=1;l<=maxlev;l++){
		
		slmt->cursize[l]=0;
		slmt->table[l]=NULL;
		
		if (l==1){ //1-gram level 
			
			ndt=tbltype[l]; ndsz=nodesize(ndt);
			
			for (table_entry_pos_t p=0;p<cursize[l];p++){
				
				entry=table[l] + (table_pos_t) p * ndsz; 
				if (lookup[word(entry)]!=-1 || keepunigr){
					
					if ((slmt->cursize[l] % slmt->dict->size()) ==0)
						slmt->table[l]=(char *)realloc(slmt->table[l],((table_pos_t) slmt->cursize[l] + (table_pos_t) slmt->dict->size()) * ndsz);
					
					newentry=slmt->table[l] + (table_pos_t) slmt->cursize[l] * ndsz; 
					memcpy(newentry,entry,ndsz);
					if (!keepunigr) //do not change encoding if keepunigr is true
						slmt->word(newentry,lookup[word(entry)]);
					
					if (l<maxlev) 
						slmt->bound(newentry,ndt,p); //store in bound the entry itself (**) !!!!
					slmt->cursize[l]++;			
				}
			}
		}
		
		else{ //n-grams n>1: scan lower order table
			
			pndt=tbltype[l-1]; pndsz=nodesize(pndt);
			ndt=tbltype[l]; ndsz=nodesize(ndt);
			
			for (table_entry_pos_t p=0; p<slmt->cursize[l-1]; p++){
				
				//determine start and end of successors of this entry
				origin=slmt->bound(slmt->table[l-1] + (table_pos_t)p * pndsz,pndt); //position of n-1 gram in this table (**)
				if (origin == 0) start=0;                              //succ start at first pos in table[l]
				else start=bound(table[l-1] + (table_pos_t)(origin-1) * pndsz,pndt);//succ start after end of previous entry
					end=bound(table[l-1] + (table_pos_t)origin * pndsz,pndt);           //succ end where indicated 
					
					if (!keepunigr || lookup[word(table[l-1] + (table_pos_t)origin * pndsz)]!=-1){
						while (start < end){
							
							entry=table[l] + (table_pos_t) start * ndsz;
							
							if (lookup[word(entry)]!=-1){
								
								if ((slmt->cursize[l] % slmt->dict->size()) ==0)
									slmt->table[l]=(char *)realloc(slmt->table[l],(table_pos_t) (slmt->cursize[l]+slmt->dict->size()) * ndsz);
								
								newentry=slmt->table[l] + (table_pos_t) slmt->cursize[l] * ndsz; 
								memcpy(newentry,entry,ndsz);
								if (!keepunigr) //do not change encoding if keepunigr is true
									slmt->word(newentry,lookup[word(entry)]);
								if (l<maxlev)
									slmt->bound(newentry,ndt,start); //store in bound the entry itself!!!!
								slmt->cursize[l]++;
							} 
							
							start++;
							
						}
					}
					
					//updated bound information of incoming entry
					slmt->bound(slmt->table[l-1] + (table_pos_t) p * pndsz, pndt,slmt->cursize[l]);
					
			}						
			
		}
		
	}
	
	
	return slmt;
}



// saves a LM table in text format

void lmtable::savetxt(const char *filename){
	
  fstream out(filename,ios::out);
  table_entry_pos_t cnt[1+MAX_NGRAM];
  int l;
	
//	out.precision(7);
	out.precision(6);
			
  if (isQtable){
    out << "qARPA " << maxlev;
    for (l=1;l<=maxlev;l++)
      out << " " << NumCenters[l];
    out << endl;
  }
	
  ngram ng(lmtable::getDict(),0);
	
  cerr << "savetxt: " << filename << "\n";
	
  if (isPruned) ngcnt(cnt); //check size of table by considering pruned n-grams
	
  out << "\n\\data\\\n";
  for (l=1;l<=maxlev;l++){
    out << "ngram " << l << "= " << (isPruned?cnt[l]:cursize[l]) << "\n";
  }
	
  for (l=1;l<=maxlev;l++){
		
    out << "\n\\" << l << "-grams:\n";
    cerr << "save: " << (isPruned?cnt[l]:cursize[l]) << " " << l << "-grams\n";
    if (isQtable){
      out << NumCenters[l] << "\n";
      for (int c=0;c<NumCenters[l];c++){
        out << Pcenters[l][c];
        if (l<maxlev) out << " " << Bcenters[l][c];
        out << "\n";
      }
    }
		
    ng.size=0;
    dumplm(out,ng,1,l,0,cursize[1]);
		
  }
	
  out << "\\end\\\n";
  cerr << "done\n";
}



void lmtable::savebin(const char *filename){
		
  if (isPruned){
    cerr << "savebin: pruned LM cannot be saved in binary form\n";
    exit(0);
  }
		
	
  fstream out(filename,ios::out);
  cerr << "savebin: " << filename << "\n";
	
  // print header
	if (isQtable){
		out << "Qblmt" << (isInverted?"I":"") << " " << maxlev;
		for (int i=1;i<=maxlev;i++) out << " " << cursize[i];
		out << "\nNumCenters";
		for (int i=1;i<=maxlev;i++)  out << " " << NumCenters[i];
		out << "\n";
		
	}else{
    out << "blmt" << (isInverted?"I":"") << " " << maxlev;
    for (int i=1;i<=maxlev;i++) out << " " << cursize[i] ;
    out << "\n";
  }
	
  lmtable::getDict()->save(out);
	
  for (int i=1;i<=maxlev;i++){
    cerr << "saving " << cursize[i] << " " << i << "-grams\n";
    if (isQtable){
      out.write((char*)Pcenters[i],NumCenters[i] * sizeof(float));
      if (i<maxlev)
        out.write((char *)Bcenters[i],NumCenters[i] * sizeof(float));
    }
    out.write(table[i],(table_pos_t) cursize[i]*nodesize(tbltype[i]));
  }
	
  cerr << "done\n";
}

void lmtable::savebin_dict(std::fstream& out){
	/*
  if (isPruned){
    cerr << "savebin_dict: pruned LM cannot be saved in binary form\n";
    exit(0);
  }
	 */
		 
  cerr << "savebin_dict ...\n";
  getDict()->save(out);
}



void lmtable::savebin_level(int level, const char* outfilename, int mmap){
  if (mmap>0)
    savebin_level_mmap(level, outfilename);
  else {
    savebin_level_nommap(level, outfilename);
  }
}

void lmtable::savebin_level_nommap(int level, const char* outfilename){
		
	/*
  if (isPruned){
    cerr << "savebin_level (level " << level << "):  pruned LM cannot be saved in binary form\n";
    exit(0);
  }
	 */
	 
	 assert(level<=maxlev);

	char nameNgrams[BUFSIZ];
	sprintf(nameNgrams,"%s-%dgrams",outfilename,level);
	fstream out(nameNgrams, ios::out);
	
	 // print header
	if (isQtable){
//NOT IMPLEMENTED
	}else{ 
		//do nothing
	}

	cerr << "saving " << cursize[level] << " " << level << "-grams in " << nameNgrams << std::endl;

	if (isQtable){
		//NOT IMPLEMENTED
	}
	out.write(table[level],(table_pos_t) cursize[level]*nodesize(tbltype[level]));
	out.close();

  cerr << "done\n";
}

void lmtable::savebin_level_mmap(int level, const char* outfilename){
	char nameNgrams[BUFSIZ];
	sprintf(nameNgrams,"%s-%dgrams",outfilename,level);
	
	cerr << "saving " << level << "-grams probs in " << nameNgrams << " (Actually do nothing)" <<std::endl;
}



void lmtable::print_table_stat(){
	cerr << "printing statistics of tables" << std::endl;
	for (int i=1;i<=maxlev;i++)
		print_table_stat(i);
}

void lmtable::print_table_stat(int level){
	cerr << " level: " << level 
	<< " maxsize[level]:" << maxsize[level] 
	<< " cursize[level]:" << cursize[level] 
	<< " table[level]:" << (void*) table[level]
	<< " tableGaps[level]:" << (void*) tableGaps[level]
	<< std::endl;
}


void lmtable::compact_level(int level, const char* outfilename){
	char nameNgrams[BUFSIZ];
	sprintf(nameNgrams,"%s-%dgrams",outfilename,level);
	
	cerr << "concatenating " << level << "-grams probs from " << nameNgrams << " to " << outfilename<< std::endl;
	
	
	//concatenating of new table to the existing data
	char cmd[BUFSIZ];
	sprintf(cmd,"cat %s >> %s", nameNgrams, outfilename);
	system(cmd);
	
	//removing temporary files
	cerr << "removing " << nameNgrams << std::endl;
	sprintf(cmd,"rm %s", nameNgrams);
	system(cmd);	
}

void lmtable::resize_level(int level, const char* outfilename, int mmap){
  if (mmap>0)
		resize_level_mmap(level, outfilename);
  else {
		if (level<maxlev) // (apart from last level maxlev, because is useless), resizing is done when saving
			resize_level_nommap(level);
  }
}

void lmtable::resize_level_mmap(int level, const char* outfilename){
	//getting the level-dependent filename
	char nameNgrams[BUFSIZ];
	sprintf(nameNgrams,"%s-%dgrams",outfilename,level);
	
	//recompute exact filesize
	table_pos_t filesize=(table_pos_t) cursize[level] * nodesize(tbltype[level]);
	cerr << level << "-grams: resizing table from " << maxsize[level] << " to " << cursize[level] << " entries (" << filesize << " bytes)" << std::endl;
		
	//opening output file
	FILE *fd = NULL;
	fd = fopen(nameNgrams, "r+");
		
	// set the file to the proper size:
	Munmap(table[level]-tableGaps[level],(table_pos_t) filesize+tableGaps[level],0);
	ftruncate(fileno(fd),filesize);
	table[level]=(char *)(MMap(fileno(fd),PROT_READ|PROT_WRITE,0,filesize,&tableGaps[level]));
	maxsize[level]=cursize[level];
}

void lmtable::resize_level_nommap(int level){
	//recompute exact filesize
	table_pos_t filesize=(table_pos_t) cursize[level] * nodesize(tbltype[level]);
	cerr << level << "-grams: resizing table from " << maxsize[level] << " to " << cursize[level] << " entries (" << filesize << " bytes)" << std::endl;
	char* ptr = new char[filesize];
	memcpy(ptr,table[level],filesize);
	delete table[level];
	table[level]=ptr;
	maxsize[level]=cursize[level];
}

//manages the long header of a bin file
//and allocates table for each n-gram level

void lmtable::loadbinheader(istream& inp,const char* header){
	
	// read rest of header
	inp >> maxlev;
	
	if (strncmp(header,"Qblmt",5)==0){ 
		isQtable=1;
		if (strncmp(header,"QblmtI",6)==0)
			isInverted=1;
	}
	else if(strncmp(header,"blmt",4)==0){ 
		isQtable=0;
		if (strncmp(header,"blmtI",5)==0)
			isInverted=1;
	}
	else error((char*)"loadbin: LM file is not in binary format");
	
	configure(maxlev,isQtable);
	
	for (int l=1;l<=maxlev;l++){
		inp >> cursize[l]; maxsize[l]=cursize[l];
	}
	
	char header2[MAX_LINE];
	if (isQtable){
		inp >> header2;
		for (int i=1;i<=maxlev;i++){
			inp >> NumCenters[i];
			cerr << "reading  " << NumCenters[i] << " centers\n";
		}
	}
	inp.getline(header2, MAX_LINE);
}

void lmtable::loadbinheader(istream& inp,const char* header,int level){
	
	
	cerr << "header:" << header << endl;
	if (strncmp(header,"Qblmt",5)==0){ 
		isQtable=1;
	}
	else if(strncmp(header,"blmt",4)==0){ 
		isQtable=0;
	}
	else error((char*)"loadbin: LM file is not in binary format");
	
	// read rest of header of ONE level of a binary LM
	int actuallevel;
	inp >> actuallevel;
	
	if (actuallevel!=level) error((char*)"loadbinheader: mismatch between the level in the file and the required level.");

	inp >> cursize[level];
	maxsize[level]=cursize[level];
	
	char header2[MAX_LINE];
	if (isQtable){
		inp >> header2 >> NumCenters[level];
		cerr << "reading  " << NumCenters[level] << " centers\n";
	}
	
	inp.getline(header2, MAX_LINE);
}

//load codebook of level l

void lmtable::loadbincodebook(istream& inp,int l){
	
  Pcenters[l]=new float [NumCenters[l]];
  inp.read((char*)Pcenters[l],NumCenters[l] * sizeof(float));
  if (l<maxlev){
    Bcenters[l]=new float [NumCenters[l]];
    inp.read((char *)Bcenters[l],NumCenters[l]*sizeof(float));
  }
	
}


//load a binary lmfile

void lmtable::loadbin(istream& inp, const char* header,const char* filename,int mmap){
	
  cerr << "loadbin()\n";
  loadbinheader(inp,header);
  lmtable::getDict()->load(inp);

		//if MMAP is used, then open the file
  if (filename && mmap>0){
		
#ifdef WIN32
    error("lmtable::loadbin mmap facility not yet supported under WIN32\n");
#else
		
    if (mmap <= maxlev) memmap=mmap;
    else error((char*)"keep_on_disk value is out of range\n");
		
    if ((diskid=open(filename, O_RDONLY))<0){
      std::cerr << "cannot open " << filename << "\n";
      error((char*)"dying");
    }
		
    //check that the LM is uncompressed
    char miniheader[4];
    read(diskid,miniheader,4);
    if (strncmp(miniheader,"Qblm",4) && strncmp(miniheader,"blmt",4))
      error((char*)"mmap functionality does not work with compressed binary LMs\n");
#endif
  }
	
  for (int l=1;l<=maxlev;l++){
    if (isQtable) loadbincodebook(inp,l);
    if ((memmap == 0) || (l < memmap)){
      cerr << "loading " << cursize[l] << " " << l << "-grams\n";
      table[l]=new char[(table_pos_t) cursize[l] * nodesize(tbltype[l])];
      inp.read(table[l],(table_pos_t) cursize[l] * nodesize(tbltype[l]));
			
    }
    else{
			
#ifdef WIN32
      error((char*)"mmap not available under WIN32\n");
#else
      cerr << "mapping " << cursize[l] << " " << l << "-grams\n";
      tableOffs[l]=inp.tellg();
      table[l]=(char *)MMap(diskid,PROT_READ,
                            tableOffs[l], (table_pos_t) cursize[l]*nodesize(tbltype[l]),
														&tableGaps[l]);
      table[l]+=(table_pos_t) tableGaps[l];
      inp.seekg((table_pos_t) cursize[l]*nodesize(tbltype[l]),ios_base::cur);
#endif
			
    }
  };
	
  cerr << "done\n";
	
}


//load only the dictionary of a binary lmfile
void lmtable::loadbin_dict(istream& inp, const char* header,const char* filename, int /* unused parameter: mmap */){
  cerr << "lmtable::loadbin_dict(): " << filename << " (header: " << header << ")\n";
  lmtable::getDict()->load(inp);
  cerr << "dict->size(): " << lmtable::getDict()->size() << "\n";
}

//load ONE level of a binary lmfile
void lmtable::loadbin_level(istream& inp, const char* header,int level,const char* filename,int mmap){
	
  cerr << "loadbin_level (level " << level << "): " << filename << "\n";
  loadbinheader(inp,header,level);
	
  //if MMAP is used, then open the file
  if (filename && mmap>0){
		
#ifdef WIN32
    error("lmtable::loadbin mmap facility not yet supported under WIN32\n");
#else
		
    if (mmap <= maxlev) memmap=mmap;
    else error((char*)"keep_on_disk value is out of range\n");
		
    if ((diskid=open(filename, O_RDONLY))<0){
      std::cerr << "cannot open " << filename << "\n";
      error((char*)"dying");
    }
		
    //check that the LM is uncompressed
    char miniheader[4];
    read(diskid,miniheader,4);
    if (strncmp(miniheader,"Qblm",4) && strncmp(miniheader,"blmt",4))
      error((char*)"mmap functionality does not work with compressed binary LMs\n");
#endif
  }
	
    if (isQtable) loadbincodebook(inp,level);
    if ((memmap == 0) || (level < memmap)){
      cerr << "loading " << cursize[level] << " " << level << "-grams\n";
      table[level]=new char[(table_pos_t) cursize[level] * nodesize(tbltype[level])];
			
			inp.read(table[level],(table_pos_t) cursize[level] * nodesize(tbltype[level]));		}
    else{
			
#ifdef WIN32
      error((char*)"mmap not available under WIN32\n");
#else
      cerr << "mapping " << cursize[level] << " " << level << "-grams\n";
      tableOffs[level]=inp.tellg();
      table[level]=(char *)MMap(diskid,PROT_READ,
                            tableOffs[level], (table_pos_t) cursize[level]*nodesize(tbltype[level]),
														&tableGaps[level]);
      table[level]+=(table_pos_t) tableGaps[level];
      cerr << "tableOffs " << tableOffs[level] << " tableGaps" << tableGaps[level] << "-grams\n";
      inp.seekg((table_pos_t) cursize[level]*nodesize(tbltype[level]),ios_base::cur);
#endif
			
    }
	
  cerr << "done\n";
	
}

int lmtable::get(ngram& ng,int n,int lev){
//if (debug) std::cout << "lmtable::get ng:|" << ng << "|" << std::endl;
  /***
std::cout << "lmtable::get ng:|" << ng << "|" << std::endl;
	cout << "cerco:" << ng << "\n";
  ***/
  totget[lev]++;
	
  if (lev > maxlev) error((char*)"get: lev exceeds maxlevel");
  if (n < lev) error((char*)"get: ngram is too small");
	
  //set boudaries for 1-gram
  table_entry_pos_t offset=0,limit=cursize[1];
	
  //information of table entries
  table_entry_pos_t hit;
  char* found; LMT_TYPE ndt;
  ng.link=NULL;
  ng.lev=0;
	
  for (int l=1;l<=lev;l++){
		
    //initialize entry information
    hit = 0 ; found = NULL; ndt=tbltype[l];

#ifdef LMT_CACHE_ENABLE
    if (lmtcache[l] && lmtcache[l]->get(ng.wordp(n),found))
    {
      hit=1;
    }
    else
    {
	search(l,
             offset,
             (limit-offset),
             nodesize(ndt),
             ng.wordp(n-l+1),
             LMT_FIND,
             &found);
    }   



   //insert both found and not found items!!!
   //insert only not found items!!!
    if (lmtcache[l] && hit==0)
    {

      const char* found2=found;
      lmtcache[l]->add(ng.wordp(n),found2);
    }
#else
      search(l,
             offset,
             (limit-offset),
             nodesize(ndt),
             ng.wordp(n-l+1),
             LMT_FIND,
             &found);
#endif

    if (!found) return 0;

    float pr = prob(found,ndt);
    if (pr==NOPROB) return 0; //pruned n-gram

    ng.path[l]=found; //store path of found entries
    ng.bow=(l<maxlev?bow(found,ndt):0);
    ng.prob=pr;
    ng.link=found;
    ng.info=ndt;
    ng.lev=l;
		
    if (l<maxlev){ //set start/end point for next search
			
      //if current offset is at the bottom also that of successors will be
      if (offset+1==cursize[l]) limit=cursize[l+1];
      else limit=bound(found,ndt);
			
      //if current start is at the begin, then also that of successors will be
      if (found==table[l]) offset=0;
      else offset=bound((found - nodesize(ndt)),ndt);
			
      assert(offset!=BOUND_EMPTY1);
      assert(limit!=BOUND_EMPTY1);
    }
  }

	
  //put information inside ng
  ng.size=n;  ng.freq=0;
  ng.succ=(lev<maxlev?limit-offset:0);
	
#ifdef TRACE_CACHELM
  if (ng.size==maxlev && sentence_id>0){
    *cacheout << sentence_id << " miss " << ng << " " << ng.link << "\n";
  }
#endif
  return 1;
}


//recursively prints the language model table

void lmtable::dumplm(fstream& out,ngram ng, int ilev, int elev, table_entry_pos_t ipos,table_entry_pos_t epos){

	LMT_TYPE ndt=tbltype[ilev];
	ngram ing(ng.dict);
	int ndsz=nodesize(ndt);
	
	assert(ng.size==ilev-1);
//Note that ipos and epos are always large r tahn or equal to 0 because they are unsigned int
	assert(epos<=cursize[ilev] && ipos<epos);
	ng.pushc(0);
	
	for (table_entry_pos_t i=ipos;i<epos;i++){
		*ng.wordp(1)=word(table[ilev]+(table_pos_t) i*ndsz);
		
		float ipr=prob(table[ilev]+(table_pos_t) i*ndsz,ndt);
		//int ipr=prob(table[ilev]+i*ndsz,ndt);
		
		//skip pruned n-grams
		if(isPruned && ipr==NOPROB) continue;
		
		if (ilev<elev){
			//get first and last successor position
			table_entry_pos_t isucc=(i>0?bound(table[ilev]+ (table_pos_t) (i-1) * ndsz,ndt):0);
			table_entry_pos_t esucc=bound(table[ilev]+ (table_pos_t) i * ndsz,ndt);
			if (isucc < esucc) //there are successors!
				dumplm(out,ng,ilev+1,elev,isucc,esucc);
			//else
			//cout << "no successors for " << ng << "\n";
		}
		else{
			//out << i << " "; //this was just to count printed n-grams
			out << ipr <<"\t";
			//out << (isQtable?ipr:*(float *)&ipr) <<"\t";
			
			// if table is inverted then revert n-gram
			if (isInverted & ng.size>1){
				ing.invert(ng);
				ng=ing;
			}
			
			for (int k=ng.size;k>=1;k--){
				if (k<ng.size) out << " ";
				out << lmtable::getDict()->decode(*ng.wordp(k));
			}
			
			if (ilev<maxlev){
				float ibo=bow(table[ilev]+ (table_pos_t)i * ndsz,ndt);
				if (isQtable) out << "\t" << ibo;
				else if (ibo!=0.0) out << "\t" << ibo;
				/*
				 int ibo=bow(table[ilev]+ i * ndsz,ndt);
				 if (isQtable) out << "\t" << ibo;
				 else
				 if (*((float *)&ibo)!=0.0)
				 out << "\t" << *((float *)&ibo);
				 */
			}
			out << "\n";
		}
	}
}

//succscan iteratively returns all successors of an ngram h for which
//get(h,h.size,h.size) returned true.

int lmtable::succscan(ngram& h,ngram& ng,LMT_ACTION action,int lev){
  assert(lev==h.lev+1 && h.size==lev && lev<=maxlev);
	
  LMT_TYPE ndt=tbltype[h.lev];
  int ndsz=nodesize(ndt);
	
  table_entry_pos_t offset;
  switch (action){
		
    case LMT_INIT:
      //reset ngram local indexes
			
      ng.size=lev;
      ng.trans(h);
      //get number of successors of h
      ng.midx[lev]=0;
      offset=(h.link>table[h.lev]?bound(h.link-ndsz,ndt):0);
      h.succ=bound(h.link,ndt)-offset;
      h.succlink=table[lev]+(table_pos_t) offset * nodesize(tbltype[lev]);
      return 1; 
			
    case LMT_CONT:
      if (ng.midx[lev] < h.succ){
        //put current word into ng
        *ng.wordp(1)=word(h.succlink+(table_pos_t) ng.midx[lev]*nodesize(tbltype[lev]));
        ng.midx[lev]++;
        return 1;
      }
      else
        return 0;
			
    default:
      cerr << "succscan: only permitted options are LMT_INIT and LMT_CONT\n";
      exit(0);
  }
}

//maxsuffptr returns the largest suffix of an n-gram that is contained
//in the LM table. This can be used as a compact representation of the
//(n-1)-gram state of a n-gram LM. if the input k-gram has k>=n then it
//is trimmed to its n-1 suffix.

//non recursive version
const char *lmtable::maxsuffptr(ngram ong, unsigned int* size){
//  cerr << "lmtable::maxsuffptr\n";
//  cerr << "ong: " << ong << " -> ong.size: " << ong.size << "\n";

  if (ong.size==0){
    if (size!=NULL) *size=0;
    return (char*) NULL;
  }

  if (isInverted){
    if (ong.size>maxlev) ong.size=maxlev; //if larger mthan maxlen reduce size
    ngram ing=ong; //inverted ngram
                
    ing.invert(ong);

    //cout << "ngram:" << ing << "\n";      
    get(ing,ing.size,ing.size); // dig in the trie
    if (ing.lev > 0){ //found something?
      unsigned int isize = MIN(ing.lev,(ing.size-1)); //find largest n-1 gram suffix                  
      if (size!=NULL)  *size=isize;
      return ing.path[isize];                        
    }else{ // means a real unknown word!    
      if (size!=NULL)  *size=0;     //default statesize for zero-gram! 
      return NULL; //default stateptr for zero-gram! 
    }
  }else{
    if (ong.size>0) ong.size--; //always reduced by 1 word

    if (ong.size>=maxlev) ong.size=maxlev-1; //if still larger or equals to maxlen reduce again

    if (size!=NULL) *size=ong.size; //will return the largest found ong.size

    for (ngram ng=ong;ng.size>0;ng.size--){
      if (get(ng,ng.size,ng.size)){
        if (ng.succ==0) (*size)--;
        if (size!=NULL) *size=ng.size;
        return ng.link;
      }
    }
    if (size!=NULL) *size=0;
    return NULL;
  }
}


const char *lmtable::cmaxsuffptr(ngram ong, unsigned int* size){
  //cerr << "lmtable::CMAXsuffptr\n";
  //cerr << "ong: " << ong
  //	<< " -> ong.size: " << ong.size << "\n";
	
  if (size!=NULL) *size=ong.size; //will return the largest found ong.size
  if (ong.size==0) return (char*) NULL;
	
  char* found;
  unsigned int isize; //internal state size variable

#ifdef PS_CACHE_ENABLE
  prob_and_state_t pst;

  size_t orisize=ong.size;
  if (ong.size>=maxlev) ong.size=maxlev-1;

  //cache hit
  if (prob_and_state_cache && (ong.size==maxlev-1) && prob_and_state_cache->get(ong.wordp(maxlev-1),pst)){
        *size=pst.statesize;
        return pst.state;
  }
  ong.size = orisize;
#endif

  //cache miss
  found=(char *)maxsuffptr(ong,&isize);
	
#ifdef PS_CACHE_ENABLE	
  //cache insert
  if (ong.size>=maxlev) ong.size=maxlev-1;
  if (prob_and_state_cache && ong.size==maxlev-1){
    pst.state=found;
    pst.statesize=isize;
    prob_and_state_cache->add(ong.wordp(maxlev-1),pst);
  }
#endif
	
  if (size!=NULL) *size=isize;
  
  return found;
}



//returns log10prob of n-gram
//bow: backoff weight
//bol: backoff level

//non recursive version, also includes maxsuffptr 
double lmtable::lprob(ngram ong,double* bow, int* bol, char** maxsuffptr,unsigned int* statesize){

	if (ong.size==0) return 0.0; //sanity check
	if (ong.size>maxlev) ong.size=maxlev; //adjust n-gram level to table size
	
	if (bow) *bow=0; //initialize back-off weight
	if (bol) *bol=0; //initialize bock-off level	
	
	
	double rbow=0,lpr=0; //output back-off weight and logprob 
	float ibow,iprob;  //internal back-off weight and logprob
	
	
	if (isInverted){
		ngram ing=ong; //inverted ngram
		
		ing.invert(ong);
		//cout << "ngram:" << ing << "\n";	
		get(ing,ing.size,ing.size); // dig in the trie
		if (ing.lev >0){ //found something?
			iprob=ing.prob;
			lpr = (double)(isQtable?Pcenters[ing.size][(qfloat_t)iprob]:iprob);
			if (*ong.wordp(1)==dict->oovcode()) lpr-=logOOVpenalty; //add OOV penalty
			if (statesize)  *statesize=MIN(ing.lev,(ing.size-1)); //find largest n-1 gram suffix 
			if (maxsuffptr) *maxsuffptr=ing.path[MIN(ing.lev,(ing.size-1))];			
		}else{ // means a real unknown word!	
			lpr=-log(UNIGRAM_RESOLUTION)/M_LN10;
			if (statesize)  *statesize=0;     //default statesize for zero-gram! 
			if (maxsuffptr) *maxsuffptr=NULL; //default stateptr for zero-gram! 
		}
		
		if (ing.lev < ing.size){ //compute backoff weight
			int depth=(ing.lev>0?ing.lev:1); //ing.lev=0 (real unknown word) is still a 1-gram 
			if (bol) *bol=ing.size-depth;
			ing.size--; //get n-gram context
			get(ing,ing.size,ing.size); // dig in the trie
			if (ing.lev>0){//found something?
										 //collect back-off weights
				for (int l=depth;l<=ing.lev;l++){
					//start from first back-off level
					assert(ing.path[l]!=NULL); //check consistency of table
					ibow=this->bow(ing.path[l],tbltype[l]);
					rbow+= (double) (isQtable?Bcenters[l][(qfloat_t)ibow]:ibow);
					//avoids bad quantization of bow of <unk>
					if (isQtable && (*ing.wordp(1)==dict->oovcode())) 
						rbow-=(double)Bcenters[l][(qfloat_t)ibow];
				}
			}
		}
		
		if (bow) (*bow)=rbow;
		return rbow + lpr;
	}
	else{
		
		for (ngram ng=ong;ng.size>0;ng.size--){
			
			if (get(ng,ng.size,ng.size)){
				iprob=ng.prob;
				lpr = (double)(isQtable?Pcenters[ng.size][(qfloat_t)iprob]:iprob);
				if (*ng.wordp(1)==dict->oovcode()) lpr-=logOOVpenalty; //add OOV penalty
				if (maxsuffptr || statesize){ //one extra step is needed if ng.size=ong.size
					if (ong.size==ng.size){
						ng.size--;
						get(ng,ng.size,ng.size);
					}
					if (statesize)  *statesize=ng.size;
					if (maxsuffptr) *maxsuffptr=ng.link; //we should check ng.link != NULL   
				}
				return rbow+lpr; 
			}else{
				if (ng.size==1){ //means a real unknow word!
					if (maxsuffptr) *maxsuffptr=NULL; //default stateptr for zero-gram! 
					if (statesize)  *statesize=0;
					return rbow -log(UNIGRAM_RESOLUTION)/M_LN10;
				}
				else{ //compute backoff
					if (bol) (*bol)++; //increase backoff level
					if (ng.lev==(ng.size-1)){ //if search stopped at previous level
						ibow=ng.bow;
						rbow+= (double) (isQtable?Bcenters[ng.lev][(qfloat_t)ibow]:ibow);
						//avoids bad quantization of bow of <unk>
						if (isQtable && (*ng.wordp(2)==dict->oovcode())) 
							rbow-=(double)Bcenters[ng.lev][(qfloat_t)ibow];
					}
					if (bow) (*bow)=rbow;
				}
				
			}
			
		}
	}
	assert(0); //never pass here!!!
	return 1.0;
}	


//return log10 probsL use cache memory
double lmtable::clprob(ngram ong,double* bow, int* bol, char** state,unsigned int* statesize){
#ifdef TRACE_CACHELM
  if (probcache && ong.size==maxlev && sentence_id>0){
    *cacheout << sentence_id << " " << ong << "\n";
  }
#endif

  if (ong.size==0){
	if (statesize!=NULL) *statesize=0;
	if (state!=NULL) *state=NULL;
	return 0.0;
  }
        
  if (ong.size>maxlev) ong.size=maxlev; //adjust n-gram level to table size

#ifdef PS_CACHE_ENABLE
  double logpr = 0.0;
//cache hit
  prob_and_state_t pst_get;

  if (prob_and_state_cache && ong.size==maxlev && prob_and_state_cache->get(ong.wordp(maxlev),pst_get)){
    logpr=pst_get.logpr;
    if (bow) *bow = pst_get.bow;
    if (bol) *bol = pst_get.bol;
    if (state) *state = pst_get.state;
    if (statesize) *statesize = pst_get.statesize;

    return logpr;
  }
        
//cache miss

  prob_and_state_t pst_add;
  logpr = pst_add.logpr = lmtable::lprob(ong, &(pst_add.bow), &(pst_add.bol), &(pst_add.state), &(pst_add.statesize));


  if (bow) *bow = pst_add.bow;
  if (bol) *bol = pst_add.bol;
  if (state) *state = pst_add.state;
  if (statesize) *statesize = pst_add.statesize;


  if (prob_and_state_cache && ong.size==maxlev){
    prob_and_state_cache->add(ong.wordp(maxlev),pst_add);
  }
  return logpr;
#else
  return lmtable::lprob(ong, bow, bol, state, statesize);
#endif
};


//return log10 probsL use cache memory
//this functions simulates the clprob(ngram, ...) but it takes as input an array of codes instead of the ngram
double lmtable::clprob(int* codes, int sz, double* bow, int* bol, char** state,unsigned int* statesize){
#ifdef TRACE_CACHELM
  if (probcache && sz==maxlev && sentence_id>0){
    *cacheout << sentence_id << "\n";
    //print the codes of the vector ng
  }
#endif

  if (sz==0){
        if (statesize!=NULL) *statesize=0;
        if (state!=NULL) *state=NULL;
        return 0.0;
  }

  if (sz>maxlev) sz=maxlev; //adjust n-gram level to table size

  double logpr = 0.0;

#ifdef PS_CACHE_ENABLE
//cache hit
  prob_and_state_t pst_get;

  if (prob_and_state_cache && sz==maxlev && prob_and_state_cache->get(codes,pst_get)){
    logpr=pst_get.logpr;
    if (bow) *bow = pst_get.bow;
    if (bol) *bol = pst_get.bol;
    if (state) *state = pst_get.state;
    if (statesize) *statesize = pst_get.statesize;

    return logpr;
  }


//create the actual ngram
  ngram ong(dict);
  ong.pushc(codes,sz);
  assert (ong.size == sz);

//cache miss
  prob_and_state_t pst_add;
  logpr = pst_add.logpr = lmtable::lprob(ong, &(pst_add.bow), &(pst_add.bol), &(pst_add.state), &(pst_add.statesize));


  if (bow) *bow = pst_add.bow;
  if (bol) *bol = pst_add.bol;
  if (state) *state = pst_add.state;
  if (statesize) *statesize = pst_add.statesize;


  if (prob_and_state_cache && ong.size==maxlev){
    prob_and_state_cache->add(ong.wordp(maxlev),pst_add);
  }
  return logpr;
#else

//create the actual ngram
  ngram ong(dict);
  ong.pushc(codes,sz);
  assert (ong.size == sz);

  return lmtable::lprob(ong, bow, bol, state, statesize);
#endif
};



void lmtable::stat(int level){
  table_pos_t totmem=0,memory;
  float mega=1024 * 1024;
	
  cout.precision(2);
	
  cout << "lmtable class statistics\n";
	
  cout << "levels " << maxlev << "\n";
  for (int l=1;l<=maxlev;l++){
    memory=(table_pos_t) cursize[l] * nodesize(tbltype[l]);
    cout << "lev " << l
			<< " entries "<< cursize[l]
			<< " used mem " << memory/mega << "Mb\n";
    totmem+=memory;
  }
	
  cout << "total allocated mem " << totmem/mega << "Mb\n";
	
  cout << "total number of get and binary search calls\n";
  for (int l=1;l<=maxlev;l++){
    cout << "level " << l << " get: " << totget[l] << " bsearch: " << totbsearch[l] << "\n";
  }
	
  if (level >1 ) lmtable::getDict()->stat();
	
}

void lmtable::reset_mmap(){
#ifndef WIN32
  if (memmap>0 and memmap<=maxlev)
    for (int l=memmap;l<=maxlev;l++){
      //std::cerr << "resetting mmap at level:" << l << "\n";
      Munmap(table[l]-tableGaps[l],(table_pos_t) cursize[l]*nodesize(tbltype[l])+tableGaps[l],0);
      table[l]=(char *)MMap(diskid,PROT_READ,
                            tableOffs[l], (table_pos_t)cursize[l]*nodesize(tbltype[l]),
                            &tableGaps[l]);
      table[l]+=(table_pos_t)tableGaps[l];
    }
#endif
}

// ng: input n-gram

// *lk: prob of n-(*bol) gram
// *boff: backoff weight vector
// *bol:  backoff level

double lmtable::lprobx(ngram	ong,
                       double	*lkp,
                       double	*bop,
                       int	*bol)
{
  double bo, lbo, pr;
  float		ipr;
  //int		ipr;
  ngram		ng(dict), ctx(dict);
	
  if(bol) *bol=0;
  if(ong.size==0) {
    if(lkp) *lkp=0;
    return 0;	// lprob ritorna 0, prima lprobx usava LOGZERO
  }
  if(ong.size>maxlev) ong.size=maxlev;
  ctx = ng = ong;
  bo=0;
  ctx.shift();
  while(!get(ng)) { // back-off
		
    //OOV not included in dictionary
    if(ng.size==1) {
      pr = -log(UNIGRAM_RESOLUTION)/M_LN10;
      if(lkp) *lkp=pr; // this is the innermost probability
      pr += bo; //add all the accumulated back-off probability
      return pr;
    }
    // backoff-probability
    lbo = 0.0; //local back-off: default is logprob 0
    if(get(ctx)){ //this can be replaced with (ng.lev==(ng.size-1))
      ipr = ctx.bow;
      lbo = isQtable?Bcenters[ng.size][(qfloat_t)ipr]:ipr;
      //lbo = isQtable?Bcenters[ng.size][ipr]:*(float*)&ipr;
    }
    if(bop) *bop++=lbo;
    if(bol) ++*bol;
    bo += lbo;
    ng.size--;
    ctx.size--;
  }
  ipr = ng.prob;
  pr = isQtable?Pcenters[ng.size][(qfloat_t)ipr]:ipr;
  //pr = isQtable?Pcenters[ng.size][ipr]:*((float*)&ipr);
  if(lkp) *lkp=pr;
  pr += bo;
  return pr;
}


// FABIO
table_entry_pos_t lmtable::wdprune(float	*thr,
																	 int	aflag)
{
  int	l;
  ngram	ng(lmtable::getDict(),0);
	
  isPruned=true;  //the table now might contain pruned n-grams
	
  ng.size=0;
  for(l=2; l<=maxlev; l++) wdprune(thr, aflag, ng, 1, l, 0, cursize[1]);
  return 0;
}

// FABIO: LM pruning method

table_entry_pos_t lmtable::wdprune(float *thr, int aflag, ngram ng, int ilev, int elev, table_entry_pos_t ipos, table_entry_pos_t	epos, double	tlk,
																	 double	bo, double	*ts, double	*tbs)
{
  LMT_TYPE	ndt=tbltype[ilev];
  int		   ndsz=nodesize(ndt);
  char		 *ndp;
  float		 lk;
  float ipr, ibo;
  //int ipr, ibo;
  table_entry_pos_t i, k, nk;
	
  assert(ng.size==ilev-1);
//Note that ipos and epos are always large r tahn or equal to 0 because they are unsigned int
  assert(epos<=cursize[ilev] && ipos<epos);
	
  ng.pushc(0); //increase size of n-gram
	
  for(i=ipos, nk=0; i<epos; i++) {
		
    //scan table at next level ilev from position ipos
    ndp = table[ilev]+(table_pos_t)i*ndsz;
    *ng.wordp(1) = word(ndp);
		
    //get probability
    ipr = prob(ndp, ndt);
    if(ipr==NOPROB) continue;	// Has it been already pruned ??
    lk = ipr;
    //lk = *(float*)&ipr;
		
    if(ilev<elev) { //there is an higher order
			
      //get backoff-weight for next level
      ibo = bow(ndp, ndt);
      bo = ibo;
      //bo = *(float*)&ibo;
			
      //get table boundaries for next level
      table_entry_pos_t isucc = i>0 ? bound(ndp-ndsz, ndt) : 0;
      table_entry_pos_t  esucc = bound(ndp, ndt);
      if(isucc>=esucc) continue; // no successors
			
      //look for n-grams to be pruned with this context (see
      //back-off weight)
prune:	double ts=0, tbs=0;
      k = wdprune(thr, aflag, ng, ilev+1, elev, isucc, esucc,
									tlk+lk, bo, &ts, &tbs);
      //k  is the number of pruned n-grams with this context
      if(ilev!=elev-1) continue;
      if(ts>=1 || tbs>=1) {
				cerr << "ng: " << ng
				<<" ts=" << ts
				<<" tbs=" << tbs
				<<" k=" << k
				<<" ns=" << esucc-isucc
				<< "\n";
				if(ts>=1) {
					pscale(ilev+1, isucc, esucc,
								 0.999999/ts);
					goto prune;
				}
      }
      // adjusts backoff:
      // 1-sum_succ(pr(w|ng)) / 1-sum_succ(pr(w|bng))
      bo = log((1-ts)/(1-tbs))/M_LN10;
			///TOCHECK: Nicola 18 dicembre 2009)
      ibo=(float)bo;
      //*(float*)&ibo=bo;
      bow(ndp, ndt, ibo);
    } else { //we are at the highest level
			
      //get probability of lower order n-gram
      ngram	bng = ng; --bng.size;
      double blk = lprob(bng);
			
      double wd = pow(10., tlk+lk) * (lk-bo-blk);
      if(aflag&&wd<0) wd=-wd;
      if(wd > thr[elev-1]) {	// kept
				*ts += pow(10., lk);
				*tbs += pow(10., blk);
      } else {		// discarded
				++nk;
				prob(ndp, ndt, NOPROB);
      }
    }
  }
  return nk;
}

int lmtable::pscale(int lev, table_entry_pos_t ipos, table_entry_pos_t epos, double s)
{
  LMT_TYPE        ndt=tbltype[lev];
  int             ndsz=nodesize(ndt);
  char            *ndp;
  float             ipr;
  //int             ipr;
	
  s=log(s)/M_LN10;
  ndp = table[lev]+ (table_pos_t) ipos*ndsz;
  for(table_entry_pos_t i=ipos; i<epos; ndp+=ndsz,i++) {
    ipr = prob(ndp, ndt);
    if(ipr==NOPROB) continue;
		///TOCHECK: Nicola 18 dicembre 2009)
    ipr+=(float) s;
    //*(float*)&ipr+=s;
    prob(ndp, ndt, ipr);
  }
  return 0;
}

//recompute table size by excluding pruned n-grams
table_entry_pos_t lmtable::ngcnt(table_entry_pos_t	*cnt)
{
  ngram	ng(lmtable::getDict(),0);
  memset(cnt, 0, (maxlev+1)*sizeof(*cnt));
  ngcnt(cnt, ng, 1, 0, cursize[1]);
  return 0;
}

//recursively compute size
table_entry_pos_t lmtable::ngcnt(table_entry_pos_t *cnt, ngram	ng, int	l, table_entry_pos_t ipos, table_entry_pos_t	epos){
	
  table_entry_pos_t	i, isucc, esucc;
  float ipr;
  //int ipr;
  char		*ndp;
  LMT_TYPE	ndt=tbltype[l];
  int		ndsz=nodesize(ndt);
	
  ng.pushc(0);
  for(i=ipos; i<epos; i++) {
    ndp = table[l]+(table_pos_t) i*ndsz;
    *ng.wordp(1)=word(ndp);
    ipr=prob(ndp, ndt);
    if(ipr==NOPROB) continue;
    ++cnt[l];
    if(l==maxlev) continue;
    isucc = (i>0)?bound(ndp-ndsz, ndt):0;
    esucc = bound(ndp, ndt);
    if(isucc < esucc) ngcnt(cnt, ng, l+1, isucc, esucc);
  }
  return 0;
}




