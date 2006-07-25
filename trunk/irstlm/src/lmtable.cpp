/*
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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

using namespace std;

#include <iostream>
#include <stdexcept>

#include "math.h"
#include "mempool.h"
#include "htable.h"
#include "dictionary.h"
#include "ngram.h"
#include "lmtable.h"


int* a;

inline void error(char* message){
  cerr << message << "\n";
  throw std::runtime_error(message);
}

lmtable::lmtable(std::istream& inp){
  
	//initialization
  maxlev=1; 

	dict=new dictionary((char *)NULL,1000000,(char*)NULL,(char*)NULL);

	//default settings is a non quantized lmtable  
	configure(1,isQtable=0);

  char header[1024];

	inp >> header; cerr << header << "\n";
  
  if (strncmp(header,"Qblmt",5)==0 || strncmp(header,"blmt",4)==0)
    loadbin(inp, header);
  else 
    loadtxt(inp, header);

  dict->genoovcode();
	
  cerr << "OOV code is " << dict->oovcode() << "\n";
}


int parseWords(char *sentence, char **words, int max)
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

void parseline(std::istream& inp, int Order,ngram& ng,float& prob,float& bow){

  char* words[1+ LMTMAXLEV + 1 + 1];
  int howmany;		
  char line[1024];

  inp.getline(line,1024);

  howmany = parseWords(line, words, Order + 3);
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
}
	
	
void lmtable::loadcenters(std::istream& inp,int Order){
  char line[11];
	  
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
  inp.getline((char*)line,10);
	  
}


void lmtable::loadtxt(std::istream& inp, const char* header){

  //open input stream and prepare an input string
  char line[1024];

  //prepare word dictionary
  //dict=(dictionary*) new dictionary(NULL,1000000,NULL,NULL); 
  dict->incflag(1);
  
  //put here ngrams, log10 probabilities or their codes
  ngram ng(dict); 
  float prob,bow,log10=(float)log(10.0);

  //check the header to decide if the LM is quantized or not
  isQtable=(strncmp(header,"qARPA",5)==0?true:false);

  //we will configure the table later we we know the maxlev;
  bool yetconfigured=false;

  cerr << "loadtxt()\n";
  
  // READ ARPA Header
  int Order, n;
  
  while (inp.getline(line,1024)){

    bool backslash = (line[0] == '\\');
    
    if (sscanf(line, "ngram %d=%d", &Order, &n) == 2) {
      maxsize[Order] = n; maxlev=Order; //upadte Order
    }

    if (backslash && sscanf(line, "\\%d-grams", &Order) == 1) {
      
      //at this point we are sure about the size of the LM
      if (!yetconfigured) {configure(maxlev,isQtable);yetconfigured=true;}			 
      
      cerr << Order << "-grams: reading ";
      
      if (isQtable) loadcenters(inp,Order);			
      
      //allocate space for loading the table of this level
      table[Order]= new char[maxsize[Order] * nodesize(tbltype[Order])];
      
      //allocate support vector to manage badly ordered n-grams
      if (maxlev>1) {
	startpos[Order]=new int[maxsize[Order]];
	for (int c=0;c<maxsize[Order];c++) startpos[Order][c]=-1;
      }
			
      //prepare to read the n-grams entries
      cerr << maxsize[Order] << " entries\n";

      //WE ASSUME A WELL STRUCTURED FILE!!!

      for (int c=0;c<maxsize[Order];c++){
	  
	parseline(inp,Order,ng,prob,bow);

	//add to table
	add(ng,
	    (int)(isQtable?prob:exp(prob * log10)*UNIGRAM_RESOLUTION),
	    (int)(isQtable?bow:exp(bow * log10)*UNIGRAM_RESOLUTION));
	}
      // now we can fix table at level Order -1
      if (maxlev>1 && Order>1) checkbounds(Order-1);			
    }
  }

 dict->incflag(0);  
 cerr << "done\n";

}

//set all bounds of entries with no successors to the bound 
//of the previous entry.

void lmtable::checkbounds(int level){

  char*  tbl=table[level];
	char*  succtbl=table[level+1];
	
  LMT_TYPE ndt=tbltype[level], succndt=tbltype[level+1];
	int ndsz=nodesize(ndt), succndsz=nodesize(succndt);
	
	//re-order table at level+1
	char* newtbl=new char[succndsz * cursize[level+1]];
	int start,end,newstart;

	//re-order table at
	newstart=0;
  for (int c=0;c<cursize[level];c++){
		start=startpos[level][c]; end=bound(tbl+c*ndsz,ndt);
   //is start==-1 there are no successors for this entry and end==-2
		if (end==-2) end=start;
		assert(start<=end);
		assert(newstart+(end-start)<=cursize[level+1]);
		assert(end<=cursize[level+1]);
	
		if (start<end)
		memcpy((void*)(newtbl + newstart * succndsz),
					 (void*)(succtbl + start * succndsz), 
					 (end-start) * succndsz);
		
		bound(tbl+c*ndsz,ndt,newstart+(end-start));
		newstart+=(end-start);
  }
	delete [] table[level+1];
	table[level+1]=newtbl;
	newtbl=NULL;
}

//Add method inserts n-grams in the table structure. It is ONLY used during 
//loading of LMs in text format. It searches for the prefix, then it adds the 
//suffix to the last level and updates the start-end positions. 

int lmtable::add(ngram& ng,int iprob,int ibow){

  char *found; LMT_TYPE ndt; int ndsz;  

  if (ng.size>1){
    
    // find the prefix starting from the first level
		int start=0, end=cursize[1]; 

    for (int l=1;l<ng.size;l++){

      ndt=tbltype[l]; ndsz=nodesize(ndt);
      
      if (search(table[l] + (start * ndsz),ndt,l,(end-start),ndsz,
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
			else{
	   cerr << "warning: missing back-off for ngram " << ng << "\n";
	return 0;
      }
		}
      
		// update book keeping information about level ng-size -1.
		// if this is the first successor update start position
		int position=(found-table[ng.size-1])/ndsz;
		if (startpos[ng.size-1][position]==-1)
			startpos[ng.size-1][position]=cursize[ng.size];

		  //always update ending position	
		bound(found,ndt,cursize[ng.size]+1);
		//cout << "startpos: " << startpos[ng.size-1][position] 
		//<< " endpos: " << bound(found,ndt) << "\n";

      }

  // just add at the end of table[ng.size]

	assert(cursize[ng.size]< maxsize[ng.size]); // is there enough space?
  ndt=tbltype[ng.size];ndsz=nodesize(ndt);
  
  found=table[ng.size] + (cursize[ng.size] * ndsz);
  word(found,*ng.wordp(1));
  prob(found,ndt,iprob);
	if (ng.size<maxlev){bow(found,ndt,ibow);bound(found,ndt,-2);}
  
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

  //prepare search pattern
  char w[LMTCODESIZE];putmem(w,ngp[0],0,LMTCODESIZE);

  int idx=0; // index returned by mybsearch
  if (found) *found=NULL;	//initialize output variable	
  switch(action){
  case LMT_FIND:
    if (!tb || !mybsearch(tb,n,sz,(unsigned char *)w,&idx)) 
      return 0;
    else
      if (found) *found=tb + (idx * sz);
    return tb + (idx * sz);
  default:
    error("lmtable::search: this option is available");
  };

  return (void *)0x0;
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


// saves a LM table in text format

void lmtable::savetxt(const char* filename){

  fstream out(filename,ios::out);
  int l;
	
  out.precision(6);			

  if (isQtable) out << "qARPA\n";	


  ngram ng(dict,0);

  cerr << "savetxt()\n";

  out << "\n\\data\\\n";
  for (l=1;l<=maxlev;l++){
    out << "ngram " << l << "= " << cursize[l] << "\n";
  }

  for (l=1;l<=maxlev;l++){
    
    out << "\n\\" << l << "-grams:\n";
    cerr << "save: " << cursize[l] << " " << l << "-grams\n";
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

  fstream out(filename,ios::out);
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


void lmtable::loadbin(std::istream& inp, const char *header){
  
  cerr << "loadbin()\n";

  // read header
  inp >> maxlev;
  
  if (strncmp(header,"Qblmt",5)==0) isQtable=1;
	else if(strncmp(header,"blmt",4)==0) isQtable=0;
	else error("loadbin: wrong header");
	
	configure(maxlev,isQtable);

  for (int i=1;i<=maxlev;i++){
    inp >> cursize[i]; maxsize[i]=cursize[i];
    table[i]=new char[cursize[i] * nodesize(tbltype[i])];
  }
  
  if (isQtable){
    cerr << "reading num centers:";
    char tmp[1024];
    inp >> tmp;
    for (int i=1;i<=maxlev;i++){
      inp >> NumCenters[i];cerr << " " << NumCenters[i];
      Pcenters[i]=new float [NumCenters[i]];
      Bcenters[i]=(i<maxlev?new float [NumCenters[i]]:NULL);
    }
    cerr << "\n";
  }

  //dict=new dictionary(NULL,1000000,NULL,NULL);
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
  
  if (lev > maxlev) error("get: lev exceeds maxlevel");
  if (n < lev) error("get: ngram is too small");

	//set boudaries for 1-gram 
  int offset=0,limit=cursize[1];

  //information of table entries
	char* found; LMT_TYPE ndt;
	
  for (int l=1;l<=lev;l++){
    
    //initialize entry information 
    found = NULL; ndt=tbltype[l];
    
    //search in table at level i
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
      
			//if current offset is at the bottom also that of successors will be
      if (offset+1==cursize[l]) limit=cursize[l+1];
      else limit=bound(found,ndt);
      
      //if current start is at the begin, then also that of successors will be
      if (found==table[l]) offset=0;
      else offset=bound((found - nodesize(ndt)),ndt);
      
      assert(offset!=-1); assert(limit!=-1);      
    }
  }
  
	//put information inside ng
  ng.size=n; ng.lev=lev; ng.freq=0; ng.link=found; ng.info=ndt;
  ng.succ=(lev<maxlev?limit-offset:0);

  return 1;
}


//recursively prints the language model table

void lmtable::dumplm(std::ostream& out,ngram ng, int ilev, int elev, int ipos,int epos){
	
  LMT_TYPE ndt=tbltype[ilev];
  int ndsz=nodesize(ndt);
  float log10=log(10.0);
	
  assert(ng.size==ilev-1);
  assert(ipos>=0 && epos<=cursize[ilev] && ipos<epos);
  ng.pushc(0);
		
  for (int i=ipos;i<epos;i++){
    *ng.wordp(1)=word(table[ilev]+i*ndsz);
    if (ilev<elev){
      //get first and last successor position
      int isucc=(i>0?bound(table[ilev]+(i-1)*ndsz,ndt):0);
      int esucc=bound(table[ilev]+i*ndsz,ndt);
      if (isucc < esucc) //there are successors!
	dumplm(out,ng,ilev+1,elev,isucc,esucc);
      //else
      //cout << "no successors for " << ng << "\n";
    }
    else{
      //out << i << " "; //this was just to count printed n-grams
      int ipr=prob(table[ilev]+ i * ndsz,ndt);
      out << (isQtable?ipr:log((ipr+1)/UNIGRAM_RESOLUTION)/log10) <<"\t";
      for (int k=ng.size;k>=1;k--){
	if (k<ng.size) out << " ";
	out << dict->decode(*ng.wordp(k));				
      }     
      int ibo=(int)(ilev<maxlev?bow(table[ilev]+ i * ndsz,ndt):UNIGRAM_RESOLUTION);      
      if (ibo!=UNIGRAM_RESOLUTION) 
	out << "\t" << (isQtable?ibo:log((ibo+1)/UNIGRAM_RESOLUTION)/log10);	
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

  default:
    cerr << "succscan: only permitted options are LMT_INIT and LMT_CONT\n";
    exit(0);
  }

}

//maxsuffptr returns the largest suffix of an n-gram that is contained 
//in the LM table. This can be used as a compact representation of the 
//(n-1)-gram state of a n-gram LM. if the input k-gram has k>=n then it 
//is trimmed to its n-1 suffix.

const char *lmtable::maxsuffptr(ngram ong){  
  
  if (ong.size==0) return (char*) NULL;
  if (ong.size>=maxlev) ong.size=maxlev-1;

  ngram ng(dict); //eventually use the <unk> word
  ng.trans(ong);

  if (get(ng,ng.size,ng.size))
    return ng.link;
  else{ 
    ong.size--;
#warning maxsuffptr is not implemented
		exit(1);
//    return getstate(ong);
  }
}


// returns the probability of an n-gram

double lmtable::prob(ngram ong){

  if (ong.size==0) return 0.0;
  if (ong.size>maxlev) ong.size=maxlev;
  
  ngram ng(dict);
  ng.trans(ong);

  double rbow;
  int ibow,iprob;
  LMT_TYPE ndt;
  
  if (get(ng,ng.size,ng.size)){
    ndt=(LMT_TYPE)ng.info; iprob=prob(ng.link,ndt);		
    return (double)(isQtable?Pcenters[ng.size][iprob]
		    :(iprob+1.0)/UNIGRAM_RESOLUTION);
  }
  else{ //size==1 means an OOV word 
    if (ng.size==1) return (double)1.0/UNIGRAM_RESOLUTION;
    else{ // compute backoff
      //set backoff state, shift n-gram, set default bow prob 
      bo_state(1); ng.shift();rbow=1.0; 			
      if (get(ng)){ 
	ndt= (LMT_TYPE)ng.info; ibow=bow(ng.link,ndt);
	rbow= (double) (isQtable?Bcenters[ng.size][ibow]:(ibow+1.0)/UNIGRAM_RESOLUTION);
      }
      //prepare recursion step
      ong.size--;
      return rbow * prob(ong);
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
