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
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include "math.h"
#include "mempool.h"
#include "htable.h"
#include "ngramcache.h"
#include "dictionary.h"
#include "n_gram.h"
#include "lmtable.h"
using namespace std;

inline void error(char* message){
  cerr << message << "\n";
  throw std::runtime_error(message);
}


//instantiate an empty lm table
lmtable::lmtable(){
  
  configure(1,false);
  
  dict=new dictionary((char *)NULL,1000000,(char*)NULL,(char*)NULL);
  
  memset(cursize, 0, sizeof(cursize));
	memset(tbltype, 0, sizeof(tbltype));
	memset(maxsize, 0, sizeof(maxsize));
	memset(info, 0, sizeof(info));
	memset(NumCenters, 0, sizeof(NumCenters));
 
  max_cache_lev=0;
  for (int i=0;i<=LMTMAXLEV+1;i++) lmtcache[i]=NULL;
  probcache=NULL;
  statecache=NULL;
  
};

 
//loadstd::istream& inp a lmtable from a lm file

void lmtable::load(istream& inp){
  
  //give a look at the header to select loading method
  char header[1024];
	
  inp >> header; cerr << header << "\n";
  
  if (strncmp(header,"Qblmt",5)==0 || strncmp(header,"blmt",4)==0)
    loadbin(inp,header);
  else 
    loadtxt(inp,header);
  
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

int parseline(istream& inp, int Order,ngram& ng,float& prob,float& bow){
	
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
  
  return 1;
}


void lmtable::loadcenters(istream& inp,int Order){
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


void lmtable::loadtxt(istream& inp,const char* header){
  
  
  //open input stream and prepare an input string
  char line[1024];
  
  //prepare word dictionary
  //dict=(dictionary*) new dictionary(NULL,1000000,NULL,NULL); 
  dict->incflag(1);
	
  //put here ngrams, log10 probabilities or their codes
  ngram ng(dict); 
  float prob,bow;;
  
  //check the header to decide if the LM is quantized or not
  isQtable=(strncmp(header,"qARPA",5)==0?true:false);
	
  //we will configure the table later we we know the maxlev;
  bool yetconfigured=false;
	
  cerr << "loadtxt()\n"; 
	
  // READ ARPA Header
  int Order,n;
  
  while (inp.getline(line,1024)){
		
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
          table[i]= new char[maxsize[i] * nodesize(tbltype[i])];                    
      }			 
      
      cerr << Order << "-grams: reading ";
			
      if (isQtable) loadcenters(inp,Order);						
			
      //allocate support vector to manage badly ordered n-grams
      if (maxlev>1 && Order<maxlev) {
        startpos[Order]=new int[maxsize[Order]];
        for (int c=0;c<maxsize[Order];c++) startpos[Order][c]=-1;
      }
			
      //prepare to read the n-grams entries
      cerr << maxsize[Order] << " entries\n";
      
      //WE ASSUME A WELL STRUCTURED FILE!!!
      
      for (int c=0;c<maxsize[Order];c++){
				
        if (parseline(inp,Order,ng,prob,bow))
          add(ng,
              (int)(isQtable?prob:*((int *)&prob)),
              (int)(isQtable?bow:*((int *)&bow)));
      }
      // now we can fix table at level Order -1
      if (maxlev>1 && Order>1) checkbounds(Order-1);			
    }
  }
	
  dict->incflag(0);  
  cerr << "done\n";
	
}


//Checkbound with sorting of n-gram table on disk

void lmtable::checkbounds(int level){
  
  char*  tbl=table[level];
  char*  succtbl=table[level+1];
	
  LMT_TYPE ndt=tbltype[level], succndt=tbltype[level+1];
  int ndsz=nodesize(ndt), succndsz=nodesize(succndt);
	
  //re-order table at level+1 on disk
  //generate random filename to avoid collisions
  char filebuff[100];char cmd[100];
  sprintf(filebuff,"/tmp/dskbuff%d_d",clock());
  fstream out(filebuff,ios::out);
  
  int start,end,newstart;
	
  //re-order table at level l+1
  newstart=0;
  for (int c=0;c<cursize[level];c++){
    start=startpos[level][c]; end=bound(tbl+c*ndsz,ndt);
    //is start==-1 there are no successors for this entry and end==-2
    if (end==-2) end=start;
    assert(start<=end);
    assert(newstart+(end-start)<=cursize[level+1]);
    assert(end<=cursize[level+1]);
		
    if (start<end)
      out.write((char*)(succtbl + start * succndsz),(end-start) * succndsz);  
    
    bound(tbl+c*ndsz,ndt,newstart+(end-start));
    newstart+=(end-start);
  }
  out.close();
  fstream inp(filebuff,ios::in);
  inp.read(succtbl,cursize[level+1]*succndsz);
  inp.close();  
  sprintf(cmd,"rm %s",filebuff);
  system(cmd);
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
	
  if (lev==1) return *found=(*ngp <n ? tb + *ngp * sz:NULL);
  
  //prepare search pattern
  char w[LMTCODESIZE];putmem(w,ngp[0],0,LMTCODESIZE);
	
  int idx=0; // index returned by mybsearch
  *found=NULL;	//initialize output variable	
  switch(action){    
    case LMT_FIND:			
      if (!tb || !mybsearch(tb,n,sz,(unsigned char *)w,&idx)) return NULL;
      else
        return *found=tb + (idx * sz);
    default:
      error("lmtable::search: this option is available");
  };
	
  return NULL;
}


int lmtable::mybsearch(char *ar, int n, int size,
                       unsigned char *key, int *idx)
{
  
  totbsearch++;
  
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

void lmtable::savetxt(const char *filename){
  
  fstream out(filename,ios::out);
  int l;
	
  out.precision(7);			
  
  if (isQtable) out << "qARPA\n";	
  
  
  ngram ng(dict,0);
  
  cerr << "savetxt: " << filename << "\n";
  
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


//manages the long header of a bin file

void lmtable::loadbinheader(istream& inp,const char* header){
  
  // read rest of header
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
    char header2[100];
    cerr << "reading num centers:";
    inp >> header2;
    for (int i=1;i<=maxlev;i++){
      inp >> NumCenters[i];cerr << " " << NumCenters[i];
      
    }
    cerr << "\n";
  } 
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

void lmtable::loadbin(istream& inp, const char* header){
    
  cerr << "loadbin()\n";
  
  loadbinheader(inp,header);
  
  dict->load(inp);  
  
  for (int l=1;l<=maxlev;l++){
    if (isQtable) loadbincodebook(inp,l);
    
    cerr << "loading " << cursize[l] << " " << l << "-grams\n";
    inp.read(table[l],cursize[l]*nodesize(tbltype[l]));
  };  
  
  cerr << "done\n";
}



int lmtable::get(ngram& ng,int n,int lev){
  
  //  cout << "cerco:" << ng << "\n";
  totget[lev]++;
  
  if (lev > maxlev) error("get: lev exceeds maxlevel");
  if (n < lev) error("get: ngram is too small");
  
  //set boudaries for 1-gram 
  int offset=0,limit=cursize[1];
	
  //information of table entries
  int hit;char* found; LMT_TYPE ndt;
  ng.link=NULL;
  ng.lev=0;            

  for (int l=1;l<=lev;l++){
		
    //initialize entry information 
    hit = 0 ; found = NULL; ndt=tbltype[l];
    
    //if (l==2) cout <<"bicache: searching:" << ng <<"\n";
    
    if (lmtcache[l] && lmtcache[l]->get(ng.wordp(n),(char *)&found))
      hit=1;
    else
      search(table[l] + (offset * nodesize(ndt)),
             ndt,
             l,
             (limit-offset),
             nodesize(ndt),
             ng.wordp(n-l+1), 
             LMT_FIND,
             &found);
 
    //insert both found and not found items!!!
    if (lmtcache[l] && hit==0)
      lmtcache[l]->add(ng.wordp(n),(char *)&found);   
    
    if (!found) return 0;      
    
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
      
      assert(offset!=-1); assert(limit!=-1);      
    }
  }
  
  //put information inside ng
  ng.size=n;  ng.freq=0; 
  ng.succ=(lev<maxlev?limit-offset:0);
  
  return 1;
}


//recursively prints the language model table

void lmtable::dumplm(fstream& out,ngram ng, int ilev, int elev, int ipos,int epos){
	
  LMT_TYPE ndt=tbltype[ilev];
  int ndsz=nodesize(ndt);
  
	
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
      out << (isQtable?ipr:*(float *)&ipr) <<"\t";
      for (int k=ng.size;k>=1;k--){
        if (k<ng.size) out << " ";
        out << dict->decode(*ng.wordp(k));				
      }     
      
      if (ilev<maxlev){
        int ibo=bow(table[ilev]+ i * ndsz,ndt);
        if (isQtable) out << "\t" << ibo;
        else
          if (*((float *)&ibo)!=0.0) 
            out << "\t" << *((float *)&ibo);
       
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
  
  ngram ng=ong;
  //ngram ng(dict); //eventually use the <unk> word
  //ng.trans(ong);
  
  if (get(ng,ng.size,ng.size))
    return ng.link;
  else{ 
    ong.size--;
    return maxsuffptr(ong);
  }
}


const char *lmtable::cmaxsuffptr(ngram ong){  
  
  if (ong.size==0) return (char*) NULL;
  if (ong.size>=maxlev) ong.size=maxlev-1;
  
  char* found;
  
  if (statecache && (ong.size==maxlev-1) && statecache->get(ong.wordp(maxlev-1),(char *)&found))
    return found;
  
  found=(char *)maxsuffptr(ong);
  
  if (statecache && ong.size==maxlev-1){
    //if (statecache->isfull()) statecache->reset();
    statecache->add(ong.wordp(maxlev-1),(char *)&found);    
  }; 
  
  return found;
  
}


// returns the probability of an n-gram

double lmtable::prob(ngram ong){
	
  if (ong.size==0) return 1.0;
  if (ong.size>maxlev) ong.size=maxlev;
  
  ngram ng(dict);
  ng.trans(ong);
	
  double rbow;
  int ibow,iprob;
  LMT_TYPE ndt;
  
	
  if (get(ng,ng.size,ng.size)){
    ndt=(LMT_TYPE)ng.info; iprob=prob(ng.link,ndt);		
    return exp((double)(isQtable?Pcenters[ng.size][iprob]:*((float *)&iprob)));
  }
  else{ //size==1 means an OOV word 
    if (ng.size==1)    
      return 1.0/UNIGRAM_RESOLUTION;
    else{ // compute backoff
          //set backoff state, shift n-gram, set default bow prob 
      bo_state(1); ng.shift();rbow=1.0; 			
      if (ng.lev==ng.size){ 
        ndt= (LMT_TYPE)ng.info; ibow=bow(ng.link,ndt);
        rbow= (double) (isQtable?Bcenters[ng.size][ibow]:*((float *)&ibow));
      }
      //prepare recursion step
      ong.size--;      
      return exp(rbow) * prob(ong);
    }
  }
}

//return log10 probs

double lmtable::lprob(ngram ong){
	
  if (ong.size==0) return 0.0;
  if (ong.size>maxlev) ong.size=maxlev;
  
  ngram ng=ong;
  //ngram ng(dict); //avoid dictionary transfer
  //ng.trans(ong);
	
  double rbow;
  int ibow,iprob;
  LMT_TYPE ndt;
  
	
  if (get(ng,ng.size,ng.size)){
    ndt=(LMT_TYPE)ng.info; iprob=prob(ng.link,ndt);		
    return (double)(isQtable?Pcenters[ng.size][iprob]:*((float *)&iprob));
  }
  else{ //size==1 means an OOV word 
    if (ng.size==1)    
      return -log(UNIGRAM_RESOLUTION)/log(10.0);
    else{ // compute backoff
          //set backoff state, shift n-gram, set default bow prob 
      bo_state(1); ng.shift();rbow=0.0; 			
      if (ng.lev==ng.size){ 
        ndt= (LMT_TYPE)ng.info; ibow=bow(ng.link,ndt);
        rbow= (double) (isQtable?Bcenters[ng.size][ibow]:*((float *)&ibow));
      }
      //prepare recursion step
      ong.size--;      
      return rbow + lprob(ong);
    }
  }
}

//return log10 probsL use cache memory

double lmtable::clprob(ngram ong){	
  
  if (ong.size==0) return 0.0;
  
  if (ong.size>maxlev) ong.size=maxlev;

  double logpr; 

#ifdef TRACE_CACHE
  if (probcache && ong.size==maxlev && sentence_id>0lo){
   *cacheout << sentence_id << " " << ong << "\n";  
  }
#endif  
  
  if (probcache && ong.size==maxlev && probcache->get(ong.wordp(maxlev),(char *)&logpr)){
    return logpr;   
  }
 
  logpr=lprob(ong);
  
  if (probcache && ong.size==maxlev){
    //if (probcache->isfull()) probcache->reset();
     probcache->add(ong.wordp(maxlev),(char *)&logpr);    
  }; 
  
  return logpr;

};



//Fill the lmtable with the n-grams in a huge lmfile.
//Use the local dictionary to select the needed ngrams 


void lmtable::filter2(const char* binlmfile, int buffMb){
  
  //load header and dictionary of binary lm on disk
  lmtable* dsklmt=new lmtable();
  fstream inp(binlmfile,ios::in);
  
  // read header
  char header[1024]; 
  inp >> header;
  
  dsklmt->loadbinheader(inp, header);
  dsklmt->dict->load(inp); 
  
  //inherit properties of the dsklmt
  configure(dsklmt->maxlevel(),dsklmt->isQuantized());
  
  //prepare word code conversion table; words which 
  //are not in the local dictionary will have code -1
  //prepare a new dictionary sorted as the large dictionary
  
  dictionary*  newdict=new dictionary((char *)NULL,1000000,(char*)NULL,(char*)NULL);
  newdict->incflag(1);  
  int* code2code=new int[dsklmt->dict->size()];
  for (int w=0;w<dsklmt->dict->size();w++){
    if (dict->getcode(dsklmt->dict->decode(w))!=-1)
      code2code[w]=newdict->encode(dsklmt->dict->decode(w));
    else code2code[w]=-1;
  }
  newdict->incflag(0);  
  delete dict;
  dict=newdict;
  
  //service variables
  char* p; char* q; char* r; 
  int ndsz; LMT_TYPE type; int w; 
  long i,j,l;
  
  
  disktable* dtbl;
  for (l=1;l<=maxlev;l++){
    
    //shortcuts for current table
    type=tbltype[l]; ndsz=nodesize(type);  
    
    //steel eventual coodebooks from dsklm;
    if (isQtable) loadbincodebook(inp,l);
    
    //allocate the maxumum number of entries to be load at each time    
    cerr << "loading part of" << dsklmt->cursize[l] << " " << l << "-grams\n";
    
    
    dtbl=new disktable(inp, (buffMb * 1024 *1024)/ndsz,ndsz,dsklmt->maxsize[l]);
    
    if (l==1){      
      
      
      //compute actual table size
      maxsize[l]=0;
      for (i=0;i<dsklmt->maxsize[l];i++){
        p=dtbl->get(inp,i);
        if ((code2code[dsklmt->word(p)]) != -1) maxsize[l]++;
      }     
      
      assert(maxsize[l]<=dsklmt->maxsize[l]);
      
      //allocate memory for table and start positions
      table[l]=new char[maxsize[l] * ndsz];
      startpos[l]=new int[maxsize[l]];
      
      //reset position of dsklmt
      dtbl->rewind(inp);
      
      //copy elements into table[l]    
      cursize[l]=0;
      for (i=0;i<dsklmt->maxsize[l];i++){
        p=dtbl->get(inp,i);
        if ((w=code2code[dsklmt->word(p)]) != -1) {
          r=table[l] + cursize[l] * ndsz;
          memcpy(r,p,ndsz); 
          //store the initial poition in startpos
          startpos[l][cursize[l]]=(i==0?0:bound(dtbl->get(inp,i-1),tbltype[l]));
          word(r,w);
          //cout << "1-gram bound:" << bound(r,tbltype[l]) << "\n";
          cursize[l]++;
        }
      }
      
      for (i=0;i<cursize[l];i++) assert(word(table[l]+i*ndsz)==i);
      
      assert(maxsize[l]==cursize[l]);        
      
    }
    else{
      
      //shortcuts for the predecessors table
      char* ptable=table[l-1]; 
      LMT_TYPE ptype=tbltype[l-1];
      int pndsz=nodesize(ptype);
      
      
      
      //count actual table size, allocate memory, and copy elements
      //we scan elements through the previous table: ptable
      //cout << inp.tellp() << "\n";      
      
      maxsize[l]=0;  
      
      for (i=0;i<cursize[l-1];i++){
        p=ptable+i*pndsz;         
        for (j=startpos[l-1][i];j<bound(p,ptype);j++){  
          assert(startpos[l-1][i]<bound(p,ptype));
          q=dtbl->get(inp,j);
          if ((w=code2code[dsklmt->word(q)]) != -1) maxsize[l]++;   
          
        }
      }  
      
      //allocate memory for the table, and fill it
      assert(maxsize[l]<=dsklmt->maxsize[l]);
      
      table[l]=new char[maxsize[l] * ndsz];
      if (l<maxlev) startpos[l]=new int[maxsize[l]];
      
      //reset position of dsklmt
      dtbl->rewind(inp);
      
      r=table[l]; //next available position in table[l]
      cursize[l]=0;
      
      for (i=0;i<cursize[l-1];i++){
        p=ptable+i*pndsz;
        for (j=startpos[l-1][i];j<bound(p,ptype);j++){  
          assert(startpos[l-1][i]<bound(p,ptype));                  
          
          q=dtbl->get(inp,j);
          
          
          if ((w=code2code[dsklmt->word(q)]) != -1){
            //copy element
            r=table[l] + cursize[l] * ndsz;
            memcpy(r,q,ndsz);
            if (l<maxlev) startpos[l][cursize[l]]=(j>0?bound(dtbl->get(inp,j-1),type):0);
            word(r,w);
            //cout << "+" << dict->decode(word(q)) << " - bound " 
            //<< startpos[l][cursize[l]] << " " << bound(p,ptype) << "\n";
            cursize[l]++; //increment index in startpos
          }
          
        }
        //update bounds of predecessor
        bound(p,ptype,cursize[l]);
      }   
      
      assert(cursize[l]==maxsize[l]);     
    }
    
    
    delete dtbl;
    if (l>1) delete [] startpos[l-1]; 
  }
  
  
}

void lmtable::filter(const char* binlmfile){
  
  //load header and dictionary of binary lm on disk
  lmtable* dsklmt=new lmtable();
  fstream inp(binlmfile,ios::in);  
  
  // read header
  char header[1024]; 
  inp >> header;
  
  dsklmt->loadbinheader(inp, header);
  
  dsklmt->dict->load(inp); 
  
  //inherit properties of the dsklmt
  configure(dsklmt->maxlevel(),dsklmt->isQuantized());
  
  //prepare word code conversion table; words which 
  //are not in the local dictionary will have code -1
  //prepare a new dictionary sorted as the large dictionary
  
  dictionary*  newdict=new dictionary((char *)NULL,1000000,(char*)NULL,(char*)NULL);
  newdict->incflag(1);  
  int* code2code=new int[dsklmt->dict->size()];
  for (int w=0;w<dsklmt->dict->size();w++){
    if (dict->getcode(dsklmt->dict->decode(w))!=-1)
      code2code[w]=newdict->encode(dsklmt->dict->decode(w));
    else code2code[w]=-1;
  }
  newdict->incflag(0);  
  delete dict;
  dict=newdict;
  
  //service variables
  char* p; char* q; char* r; 
  int ndsz; LMT_TYPE type; int w; 
  int i,j,l;
  
  for (l=1;l<=maxlev;l++){
    
    //shortcuts for current table
    type=tbltype[l]; ndsz=nodesize(type);  
    
    //steel eventual coodebooks from dsklm;
    if (isQtable) loadbincodebook(inp,l);
    
    //load single l-table from dsklmt: this table can be
    //removed at the ned of this cycle
    cerr << "loading " << dsklmt->cursize[l] << " " << l << "-grams\n";
    dsklmt->table[l]=new char[dsklmt->cursize[l]*ndsz];
    inp.read(dsklmt->table[l],dsklmt->cursize[l]*ndsz);
    
    //shortcuts for dsktable
    char* dtbl=dsklmt->table[l];
    int dsize=dsklmt->cursize[l];
    
    if (l==1){
      
      //count actual table size
      maxsize[l]=0;
      for (i=0;i<dsize;i++)    
        if ((code2code[dsklmt->word(dtbl+i*ndsz)]) != -1) maxsize[l]++;
      
      assert(maxsize[l]<=dsklmt->maxsize[l]);
      //allocate memory for table and start positions
      table[l]=new char[maxsize[l] * ndsz];
      startpos[l]=new int[maxsize[l]];
      
      //copy elements one by one
      
      for (i=0;i<dsize;i++){       
        p=dtbl+i*ndsz;
        if ((w=code2code[dsklmt->word(p)]) != -1) {
          r=table[l] + cursize[l] * ndsz;
          memcpy(r,p,ndsz); 
          //store the initial poition in startpos
          startpos[l][cursize[l]]=(i==0?0:bound(p-ndsz,tbltype[l]));
          word(r,w);
          cursize[l]++;
        }
      }
      
      for (i=0;i<cursize[l];i++) assert(word(table[l]+i*ndsz)==i);
      
      assert(maxsize[l]==cursize[l]);        
      
    }
    else{ //l>=1;
      
      //shortcuts for the predecessors table
      char* ptable=table[l-1]; 
      LMT_TYPE ptype=tbltype[l-1];
      int pndsz=nodesize(ptype);
      
      //count actual table size, allocate memory, and copy elements
      //we scan elements through the previous table: ptable
      
      maxsize[l]=0;  
      for (i=0;i<cursize[l-1];i++){
        p=ptable+i*pndsz;
        for (j=startpos[l-1][i];j<bound(p,ptype);j++){         
          q=dsklmt->table[l] + j * ndsz;
          if ((w=code2code[dsklmt->word(q)]) != -1){
            maxsize[l]++;   
          } 
        }
      }    
      
      //allocate memory for the table, and fill it
      assert(maxsize[l]<=dsklmt->maxsize[l]);
      table[l]=new char[maxsize[l] * ndsz];
      if (l<maxlev) startpos[l]=new int[maxsize[l]];
      
      r=table[l]; //next available position in table[l]
      cursize[l]=0;
      for (i=0;i<cursize[l-1];i++){
        p=ptable+i*pndsz;
        for (j=startpos[l-1][i];j<bound(p,ptype);j++){        
          q=dsklmt->table[l] + j * ndsz;
          if ((w=code2code[dsklmt->word(q)]) != -1){
            //copy element
            r=table[l] + cursize[l] * ndsz;
            memcpy(r,q,ndsz);
            if (l<maxlev)
              startpos[l][cursize[l]]=(j==0?0:dsklmt->bound(q-ndsz,type));
            word(r,w);
            //cout << "+" << dict->decode(word(q)) << " - bound " 
            //<< startpos[l][cursize[l]] << " " << bound(p,ptype) << "\n";
            cursize[l]++; //increment index in startpos
          }         
        }
        //update bounds of predecessor
        bound(p,ptype,cursize[l]);
      }    
      
    }
    
    delete [] dsklmt->table[l];
    if (l>1) delete [] startpos[l-1]; 
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
  
  cout << "total number of get calls\n";
  for (int l=1;l<=maxlev;l++){
    cout << "level " << l << " " << totget[l] << "\n";
  }
  cout << "total binary search : " << totbsearch << "\n";
  
  if (level >1 ) dict->stat();
  
}
