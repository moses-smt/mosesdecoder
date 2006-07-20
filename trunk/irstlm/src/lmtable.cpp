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

#include <iostream>
#include <fstream>
#include "math.h"
#include "mempool.h"
#include "htable.h"
#include "dictionary.h"
#include "ngram.h"
#include "lmtable.h"


lmtable::lmtable(std::istream &in, int n, int res, double dec){
  
  maxlev=n;
  dict=NULL;
  isQtable=0;

  memset(cursize, 0, sizeof(cursize));
  memset(tbltype, 0, sizeof(tbltype));
  memset(maxsize, 0, sizeof(maxsize));
  memset(info, 0, sizeof(info));
  memset(NumCenters, 0, sizeof(NumCenters));

  if (n==1){
    tbltype[1]=LEAF;
  }else if (res==0){ //non quantized
    for (int i=1;i<n;i++)
      tbltype[i]=INTERNAL;
    tbltype[n]=LEAF;
  }
  else{
    assert(res <=256);
    tbltype[1]=INTERNAL;
    for (int i=2;i<n;i++)
      tbltype[i]=QINTERNAL;
    tbltype[n]=QLEAF;
  }


  char header[1024];
  char gzip_hdr[3]; gzip_hdr[0]=0x1f; gzip_hdr[1]=0x8b; gzip_hdr[2]=0;

  in >> header;
  
  // cerr << header << "\n";

  if (strncmp(header,"Qblmt",6)==0 || strncmp(header,"blmt",4)==0)
    loadbin(in, header);
  else if (strncmp(header,"qARPA",6)==0)
    loadQtxt(in, header, maxlev);
  else if (strncmp(header, gzip_hdr, 2)==0) {
    std::cerr << "gzip'd files cannot be opened directly\n";
    std::abort();
  } else
    loadtxt(in, header, maxlev,res,dec);

  dict->genoovcode();
  cerr << "OOV code is " << dict->oovcode() << "\n";
}


unsigned int parseWords(char *sentence, char **words, unsigned int max)
{
    char *word;
    int i = 0;

    char *const wordSeparators = " \t\r\n";

    for (word = strtok(sentence, wordSeparators);
         i < max && word != 0;
         i++, word = strtok(0, wordSeparators))
    {
      words[i] = word;
    }
    if (i < max) {
        words[i] = 0;
    }

    return i;
}

	    
void lmtable::loadtxt(std::istream &inp,const char* header, int maxOrder,int res,double dec){
  
  dict=new dictionary(NULL,1000000,NULL,NULL);
  dict->incflag(1);

  ngram ng(dict); /* ngram translated to word indices */

  cerr << "loadtxt ... resolution " << res << "  decay " << dec << "\n";

  resolution=res;
  decay=dec;
  logdecay=log(decay);

  double logten=log(10.0);

  // READ ARPA Header
  
  unsigned numNgrams[LMTMAXLEV + 1]; /* # n-grams for each order */
  unsigned numRead[LMTMAXLEV + 1]; /* # n-grams read */

  float logprob,logbow;

  for (int i=1;i<=LMTMAXLEV;i++) numNgrams[i]=numRead[i]=0;

  int state = -1 ;   /* section of file being read:
		      * -1: pre-header, 0: header,
		      *  1: 1-grams, 2: 2-grams, ... */
  char line[1024];
  strncpy(line, header, 1024);

  do {  // header was already read in the calling function

    if (line[0]=='\0') continue; //skip empty 
    
    bool backslash = (line[0] == '\\');
    
    switch (state){ /* pre-header */
      
    case -1:  /* looking for start of header */
      
      if (backslash && strncmp(line, "\\data\\", 6) == 0)
	state = 0;
      
      continue;
      
    case 0:	/* ngram header */

      unsigned thisOrder;
      int nNgrams;

      if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
	/* start reading n-grams */
	
	if (state < 1 || state > LMTMAXLEV) {
	  cerr << "invalid ngram order " << state << "\n"; 
	  exit (1);
	}

	cerr << "loading " << numNgrams[state] << " " << state <<"-grams\n";
	
	continue;

      }else if (sscanf(line, "ngram %d=%d", &thisOrder, &nNgrams) == 2) {
	/* line: ngram <N>=<howmany> */
	
	if (thisOrder <= 0 || thisOrder > LMTMAXLEV) {
	  cerr << "ngram order " << thisOrder << " out of range\n";
	  exit(1);
	}
	
	if (nNgrams < 0) {
	  cerr << "ngram number " << nNgrams<< " out of range\n";
	  exit(1);
	}
	
	numNgrams[thisOrder] = nNgrams;
	maxsize[thisOrder] = nNgrams;

	table[thisOrder]= new char[nNgrams * nodesize(tbltype[thisOrder])];
	    
	continue;
      } 
      else {
	cerr << "unexpected input\n";
	exit (1);
      }
      

    default:	/* reading n-grams, where n == state */
      

      if (backslash && sscanf(line, "\\%d-grams", &state) == 1) {
	
	if (state < 1 || state > LMTMAXLEV) {
	  cerr << "invalid ngram order " << state << "\n";
	  exit(1);
	}
	
	cerr << "loading " << numNgrams[state] << " " << state <<"-grams\n";
	
	if (state>2) checkbounds(state-2);

	continue;

      } else if (backslash && strncmp(line, "\\end\\", 5) == 0) {

	if (state>1) checkbounds(state-1);

	// Check total number of ngrams read 

	for (int i = 1; i <= maxOrder; i++) {
	  if (numNgrams[i] != numRead[i]) {
	    cerr << "error: " << numRead[i] << " "
		 << i << "-grams read, expected "
		 << numNgrams[i] << "\n";
	    exit(1);
	  }
	}

      }else if (state > maxOrder){
	// skip this n-grams
	numRead[state]++;
	continue;
      }
      
      else{ // read this n-gram
	
	float prob, bow; /* probability and back-off-weight */
	
	/* Parse: <prob> <w1> <w2> ...<bow> */
	
	char* words[1+ LMTMAXLEV + 1 + 1];
	
	/* result of parsing an n-gram line
	 * the first and last elements are actually
	 * numerical parameters, but so what? */
	
	unsigned howmany = parseWords(line, words, state + 3);
	
	if (howmany < state + 1 || howmany > state + 2) {
	  cerr << "ngram line has " << howmany
				    << " fields (" << state + 2
				    << " expected)\n";
	  exit(1);
	}

	ng.size=0;
	for (int i=0;i<state;i++) 
	  ng.pushw(strcmp(words[i+1],"<unk>")?words[i+1]:ng.dict->OOV());
	
	if (!sscanf(words[0],"%f",&logprob)){
	  cerr << "bad log prob in line:\n" << line ;
	  exit (1);
	}
	
	if (state < maxOrder){ //I care about backoff 
	  
	  if (howmany==state+2){ //backoff is written
	    
	    if (!sscanf(words[state+1],"%f",&logbow)){
	      cerr << "bad log prob in line:\n" << line ;
	      exit (1);
	    }
	  }
	  else{
	    logbow=0; // backoff is implicit
	  }
	}
	  
	//cout << ng << " lp=" << logprob ;
	//if (state < maxOrder) 
	//cout << " lbo=" << logbow << "\n";
	//else 
	//cout << "\n";
	
	add(ng,logten*logprob,logten*logbow);
	numRead[state]++;
	continue;
      }
    }
  } while (inp.getline(line,1024));
  
  dict->incflag(0);  
  cerr << "done\n";

};


void lmtable::loadQtxt(std::istream &inp,const char* header,int maxOrder){
  
  dict=new dictionary(NULL,1000000,NULL,NULL);
  
  dict->incflag(1);

  ngram ng(dict); /* ngram translated to word indices */

  cerr << "loadQtxt\n";

  isQtable=1;
  
  // READ ARPA Header
  int Order,n,prcode,bowcode;

  char line[1024];
  
  while (inp.getline(line,1024)){

    bool backslash = (line[0] == '\\');
    
    if (sscanf(line, "ngram %d=%d", &Order, &n) == 2) {
      maxsize[Order] = n;
      maxlev=Order;
    }

    if (backslash && sscanf(line, "\\%d-grams", &Order) == 1) {
      
      if (Order>2) checkbounds(Order-2);      
      
      cerr << Order << "-grams: reading code book ...";
      inp >> NumCenters[Order];
      Pcenters[Order]=new float[NumCenters[Order]];
      Bcenters[Order]=(Order<maxlev?new float[NumCenters[Order]]:NULL);
      
      for (int c=0;c<NumCenters[Order];c++){
	inp >> Pcenters[Order][c];
	if (Order<maxlev) inp >> Bcenters[Order][c];
      }
      
      table[Order]= new char[maxsize[Order] * nodesize(tbltype[Order])];
      
      char word[MAX_WORD];

      cerr << " and " << maxsize[Order] << " entries\n";

      for (int c=0;c<maxsize[Order];c++){
	  
	inp >> prcode;

	ng.size=0;
	for (int i=0;i<Order;i++){ 
	  inp >> word;
	  ng.pushw(strcmp(word,"<unk>")?word:ng.dict->OOV());
	}
	
	if (Order < maxlev) inp >> bowcode;
	
	add(ng,(float)prcode,(float)bowcode);
	
      }
    }
  }

  if (maxlev>1) checkbounds(maxlev-1);

 dict->incflag(0);  
 cerr << "done\n";

};


void lmtable::checkbounds(int level){

  int b=0;
  char*  found=table[level];
  LMT_TYPE ndt=tbltype[level];

  for (int c=0;c<cursize[level];c++){
    if (bound(found,ndt)==-2)
      bound(found,ndt,b);
    else
      b=bound(found,ndt);
	
    found+= nodesize(ndt);
  }

}


int lmtable::add(ngram& ng,double logprob,double logbow){

  char *found;
  LMT_TYPE ndt;
  static int missing_backoff_warning=0;  

  static int state=1; // remember the last ngram size
  
  if (ng.size>1){
    
    // find prefix to update bounds
    int offset=0;
    int limit=cursize[1];

    for (int l=1;l<ng.size;l++){
      //      if (ng.size==3)
      //cerr << ng << " l=" << l << "\n";
      
      ndt=tbltype[l];
      
      found = NULL;
      search(table[l] + (offset * nodesize(ndt)),
	     ndt,
	     l,
	     (limit-offset),
	     nodesize(ndt),
	     ng.wordp(ng.size-l+1), 
	     LMT_FIND,
	     &found);

      if (found==NULL){
        if (missing_backoff_warning==0)
	   cerr << "warning: missing back-off for ngram " << ng << "\n";
        missing_backoff_warning=1;
	return 0;
      }
      
      if (l==ng.size-1) //update bounds
	bound(found,ndt,cursize[l+1]+1);

      else{ //set start/end point for next search
	if (offset+1==cursize[l])
	  limit=cursize[l+1];
	else
	  limit=bound(found,ndt);
      
	if (found==table[l]) //its the first element
	  offset=0;
	else
	  offset=bound((found - nodesize(ndt)),ndt);

	assert (offset!= -1);
      }

      //      if (ng.size==3)
      //cerr << "limit: " << limit << " offset " << offset << "\n";

    }
  }

  // just add at the end of table[ng.size]

  if (cursize[ng.size]>= maxsize[ng.size]) std::cerr << cursize[ng.size] << "<" << maxsize[ng.size] << std::endl;
  assert(cursize[ng.size]< maxsize[ng.size]);
  
  ndt=tbltype[ng.size];
  int iprob,ibow;

  if (isQtable){ // logprob are already quantized
    iprob=(int)logprob;
    ibow=(int)logbow;
  }
  else{
    if (ndt==QINTERNAL || ndt==QLEAF){
      iprob=resolution-(int)(logprob/logdecay)-1;
      iprob=(iprob>=0?iprob:0);
      ibow=resolution-(int)(logbow/logdecay)-1;
      ibow=(ibow>=0?ibow:0);    
    }
    else{
      iprob=(int)(exp(logprob) * UNIGRAM_RESOLUTION);
      ibow=(int)(exp(logbow) * UNIGRAM_RESOLUTION);
    }
  }

  found=table[ng.size] + (cursize[ng.size] * nodesize(ndt));
  word(found,*ng.wordp(1));
  prob(found,ndt,iprob);
  if (ng.size<maxlev){
    bow(found,ndt,ibow);
    bound(found,ndt,-2);
  }
  
  cursize[ng.size]++;
  
  return 1;
}


void *lmtable::search(char* tb,
		      LMT_TYPE ndt,
		      int lev,
		      int n,
		      int sz,
		      int *ngp,
		      LMT_ACTION action,
		      char **found){

  char w[LMTCODESIZE];
  putmem(w,ngp[0],0,LMTCODESIZE);
  int wint=ngp[0];

  // index returned by mybsearch 

  if (found) *found=NULL;

  int idx=0; 

  switch(action){
    
  case LMT_FIND:

    if (!tb || 
	!mybsearch(tb,n,sz,(unsigned char *)w,&idx))
      return 0;
    else
      if (found) *found=tb + (idx * sz);

    return tb + (idx * sz);

    break;
    
  default:
    cerr << "this option is not implemented yet\n";
    break;
  }

}



int lmtable::mybsearch(char *ar, int n, int size,
		       unsigned char *key, int *idx)
{
  register int low, high;
  register unsigned char *p;
  register int result;
  register int i;

  /* return idx with the first 
     position equal or greater than key */

  /*   Warning("start bsearch \n"); */

  low = 0;high = n; *idx=0;
  while (low < high)
    {
      *idx = (low + high) / 2;
      p = (unsigned char *) (ar + (*idx * size));
      
      //comparison
      for (i=(LMTCODESIZE-1);i>=0;i--){
	result=key[i]-p[i];
	if (result) break;
      }

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



void lmtable::savetxt(const char *filename){

  std::ofstream out(filename,ios::out);

  cerr << "savetxt: " << filename << "\n";

  out << "\n\\data\\\n";
  for (int i=1;i<=maxlev;i++){
    out << "ngram " << i << "= " << cursize[i] << "\n";
  }

  for (int i=1;i<=maxlev;i++){
    out << "\n\\" << i << "-grams:\n";
    
    cerr << "save: " << cursize[i] << " " << i << "-grams\n";

    LMT_TYPE ndt=tbltype[i];

    int nsz=nodesize(ndt);
    
    for (int c=0;c<cursize[i];c++){
      int wo=word(table[i]+ c * nsz);
      int ipr=prob(table[i]+ c * nsz,ndt);
      int ibo=(i<maxlev?bow(table[i]+ c * nsz,ndt):1);
      float pr,bo;

      out << ipr << " " << dict->decode(wo) << " " << ibo << "\n";

    }
  }
    
  out << "\\end\\\n";
  
  cerr << "done\n";
}


void lmtable::savebin(const char *filename){

  std::ofstream out(filename,ios::out);
  
  cerr << "savebin: " << filename << "\n";

  // print header
  if (isQtable){
    out << "Qblmt " << maxlev;
    for (int i=1;i<=maxlev;i++) out << " " << cursize[i];
    out << "\nNumCenters";
    for (int i=1;i<=maxlev;i++)  out << " " << NumCenters[i];
    out << "\n";
    
  }else{
    out << "blmt " << maxlev;
    for (int i=1;i<=maxlev;i++) out << " " << cursize[i] ;
    out << "\n";
    out << "resolution: " << resolution << " decay: " << decay << "\n";
  }

  dict->save(out);
  
  for (int i=1;i<=maxlev;i++){
    cerr << "saving " << cursize[i] << " " << i << "-grams\n";
    if (isQtable){
      out.write((char*)Pcenters[i],NumCenters[i] * sizeof(float));
      if (i<maxlev) 
	out.write((char *)Bcenters[i],NumCenters[i] * sizeof(float));
    }
    out.write(table[i],cursize[i]*nodesize(tbltype[i]));
  }
  
  cerr << "done\n";
}


void lmtable::loadbin(std::istream &inp,const char* h){
  
  cerr << "loadbin ... ";

  char header[1024];
  // read header
  inp >> maxlev;
  
  if (strncmp(header,"Qblmt",6)==0) isQtable=1;

  for (int i=1;i<=maxlev;i++){
    inp >> cursize[i]; maxsize[i]=cursize[i];
    table[i]=new char[cursize[i] * nodesize(tbltype[i])];
  }
  
  if (isQtable){
    cerr << "reading num centers:";
    inp >> header;
    for (int i=1;i<=maxlev;i++){
      inp >> NumCenters[i];cerr << " " << NumCenters[i];
      Pcenters[i]=new float [NumCenters[i]];
      Bcenters[i]=(i<maxlev?new float [NumCenters[i]]:NULL);
    }
    cerr << "\n";
  }else{
    inp >> header >> resolution;
    inp >> header >> decay;
    cerr << "resolution " << resolution << " decay " << decay  << "\n";
  }

  dict=new dictionary(NULL,1000000,NULL,NULL);
  dict->load(inp);  

  for (int i=1;i<=maxlev;i++){
    if (isQtable){
      inp.read((char*)Pcenters[i],NumCenters[i] * sizeof(float));
      if (i<maxlev) inp.read((char *)Bcenters[i],NumCenters[i]*sizeof(float));
    }
    cerr << "loading " << cursize[i] << " " << i << "-grams\n";
    inp.read(table[i],cursize[i]*nodesize(tbltype[i]));
  }

  cerr << "done\n";
}



int lmtable::get(ngram& ng,int n,int lev){
  
  //  cout << "cerco:" << ng << "\n";
  
  if (lev > maxlev){
    cerr << "get: lev exceeds maxlevel\n";
    exit(1);
  }

  if (n < lev){
    cerr << "get: ngram " << ng << " is too small\n";
    exit(1);
  }

  int offset=0;
  int limit=cursize[1];
  char* found;
  LMT_TYPE ndt;
  for (int l=1;l<=lev;l++){
    
    found = NULL;
    ndt=tbltype[l];
    
    search(table[l] + (offset * nodesize(ndt)),
	   ndt,
	   l,
	   (limit-offset),
	   nodesize(ndt),
	   ng.wordp(n-l+1), 
	   LMT_FIND,
	   &found);

    if (!found) return 0;

    if (l<maxlev){ //set start/end point for next search
      if (offset+1==cursize[l])
	limit=cursize[l+1];
      else
	limit=bound(found,ndt);
      
      if (found==table[l]) //its the first element
	offset=0;
      else
	offset=bound((found - nodesize(ndt)),ndt);
      
      assert(offset!=-1);
      assert(limit!=-1);
      
    }
  }
  
  ng.size=n;
  ng.lev=lev;
  ng.freq=0;
  ng.link=found;
  ng.info=ndt;
  ng.succ=(lev<maxlev?limit-offset:0);

  return 1;
}


int lmtable::succscan(ngram& h,ngram& ng,LMT_ACTION action,int lev){
  assert(lev==h.lev+1 && h.size==lev && lev<=maxlev);

  LMT_TYPE ndt=tbltype[h.lev];
  int ndsz=nodesize(ndt);
  
  switch (action){
    
  case LMT_INIT:
    //reset ngram local indexes

    ng.size=lev;
    ng.trans(h);    
    ng.midx[lev]=(h.link>table[h.lev]?bound(h.link-ndsz,ndt):0);
    
    return 1;

  case LMT_CONT:
    
    if (ng.midx[lev]<bound(h.link,ndt))
      {
	//put current word into ng
        *ng.wordp(1)=word(table[lev]+ng.midx[lev]*nodesize(tbltype[lev]));
	ng.midx[lev]++;
	return 1;
      }
    else
      return 0;
  }

}


double lmtable::prob(ngram ong){

  if (ong.size==0) return 0.0;
  if (ong.size>maxlev) ong.size=maxlev;
  
  ngram ng(dict);
  ng.trans(ong);

  double bo,pr;
  LMT_TYPE ndt;
  

  if (get(ng,ng.size,ng.size)){

    ndt= (LMT_TYPE)ng.info;

    int iprob=prob(ng.link,ndt);

    if (ndt == QINTERNAL || ndt == QLEAF)
      return (double)(isQtable?Pcenters[ng.size][iprob]:
		      pow(decay,(resolution-iprob)));
    else
      return (double)(isQtable?Pcenters[ng.size][iprob]:
		      (iprob+1)/UNIGRAM_RESOLUTION);
    
  }else{ 
    
    //OOV not included in dictionary
    if (ng.size==1)  
      return 1.0/UNIGRAM_RESOLUTION;

    // backoff-probability
    
    else{ 
    
      bo_state(1); //set backoff state to 1
      
      ng.shift();
    
      if (get(ng)){ 
	ndt= (LMT_TYPE)ng.info;
	int ibow=bow(ng.link,ndt);
	if (isQtable)
	  bo=(double)Bcenters[ng.size][ibow];
	else
	  bo=ndt==QINTERNAL?
	    (double)pow(decay,(resolution-ibow))
	    :(double)(ibow+1)/UNIGRAM_RESOLUTION;
      }
      else
	bo=1.0;
    
      ong.size--;
      
      return bo * prob(ong);
    }
  }
}



void lmtable::stat(int level){
  int totmem=0,memory;
  float mega=1024 * 1024;
  
  cout.precision(2);

  cout << "lmtable class statistics\n";
  
  cout << "levels " << maxlev << "\n";
  for (int l=1;l<=maxlev;l++){
    memory=cursize[l] * nodesize(tbltype[l]);
    cout << "lev " << l 
         << " entries "<< cursize[l] 
         << " used mem " << memory/mega << "Mb\n";
    totmem+=memory;
  }
  
  cout << "total allocated mem " << totmem/mega << "Mb\n";
  
  if (level >1 ) dict->stat();

}
