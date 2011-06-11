// $Id: ngramtable.h 34 2010-06-03 09:19:34Z nicolabertoldi $

/******************************************************************************
IrstLM: IRST Language Model Toolkit, compile LM
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

#ifndef MF_NGRAMTABLE_H
#define MF_NGRAMTABLE_H

//Backoff symbol
#ifndef BACKOFF_ 
#define BACKOFF_ "_backoff_"
#endif

//Dummy symbol
#ifndef DUMMY_ 
#define DUMMY_ "_dummy_"
#endif

// internal data structure 

#ifdef MYCODESIZE
#define DEFCODESIZE  MYCODESIZE
#else
#define DEFCODESIZE  (int)2
#endif

#define SHORTSIZE (int)2
#define PTRSIZE   (int)sizeof(char *)
#define INTSIZE   (int)4
#define CHARSIZE  (int)1


//Node  flags
#define FREQ1  (unsigned char)   1  
#define FREQ2  (unsigned char)   2
#define FREQ4  (unsigned char)   4
#define INODE  (unsigned char)   8
#define LNODE  (unsigned char)  16
#define SNODE  (unsigned char)  32
#define FREQ6 (unsigned char)   64
#define FREQ3  (unsigned char) 128

typedef char* node;  //inodes, lnodes, snodes
typedef char* table; //inode table, lnode table, singleton table

typedef unsigned char NODETYPE;


typedef enum {FIND,    //!< search: find an entry
	      ENTER,   //!< search: enter an entry 
	      DELETE,  //!< search: find and remove entry
	      INIT,    //!< scan: start scan
	      CONT     //!< scan: continue scan
} ACTION;


typedef enum {COUNT,       //!< table: only counters
	      LEAFPROB,    //!< table: only probs on leafs
	      FLEAFPROB,    //!< table: only probs on leafs and FROZEN
	      LEAFPROB2,   //!< table: only probs on leafs
	      LEAFPROB3,   //!< table: only probs on leafs
	      LEAFPROB4,   //!< table: only probs on leafs
	      LEAFCODE,    //!< table: only codes on leafs
	      SIMPLE_I,    //!< table: simple interpolated LM
	      SIMPLE_B,    //!< table: simple backoff LM
	      SHIFTBETA_I, //!< table: interpolated shiftbeta
	      SHIFTBETA_B, //!< table: backoff shiftbeta
	      MSHIFTBETA_I,//!< table: interp modified shiftbeta
	      MSHIFTBETA_B,//!< table: backoff modified shiftbeta
	      FULL,        //!< table: full fledged table

} TABLETYPE;



class tabletype{
  
  TABLETYPE ttype;

 public:

  int CODESIZE;                //sizeof word codes
  long long code_range[7]; //max code for each size

  //Offsets of internal node fields
  int WORD_OFFS;   //word code position
  int MSUCC_OFFS;  //number of successors
  int MTAB_OFFS;   //pointer to successors
  int FLAGS_OFFS;  //flag table
  int SUCC1_OFFS;  //number of successors with freq=1
  int SUCC2_OFFS;  //number of successors with freq=2
  int BOFF_OFFS;   //back-off probability
  int I_FREQ_OFFS; //frequency offset
  int I_FREQ_NUM;  //number of internal frequencies
  int L_FREQ_NUM;  //number of leaf frequencies
  int L_FREQ_SIZE; //minimum size for leaf frequencies
  
  //Offsets of leaf node fields
  int L_FREQ_OFFS; //frequency offset
  
  TABLETYPE tbtype(){return ttype;}

  tabletype(TABLETYPE tt,int codesize=DEFCODESIZE){

    if (codesize<=4 && codesize>0)
      CODESIZE=codesize;
    else{
      cerr << "ngramtable wrong codesize\n";
      exit(1);
    }
    
    code_range[1]=255;
    code_range[2]=65535;
    code_range[3]=16777214;
    code_range[4]=2147483640; 
    code_range[6]=140737488360000LL; //stay below true limit
//	code_range[6]=281474977000000LL; //stay below true limit
				
    //information which is useful to initialize 
    //LEAFPROB tables
    L_FREQ_SIZE=FREQ1; 

    WORD_OFFS  =0;        
    MSUCC_OFFS =CODESIZE;
    MTAB_OFFS  =MSUCC_OFFS+CODESIZE;
    FLAGS_OFFS =MTAB_OFFS+PTRSIZE; 
    
    switch (tt){

    case COUNT:
      SUCC1_OFFS =0;
      SUCC2_OFFS =0;
      BOFF_OFFS  =0;
      I_FREQ_OFFS=FLAGS_OFFS+CHARSIZE;
      I_FREQ_NUM=1;
      L_FREQ_NUM=1;
      
      ttype=tt;
      break;

    case FULL:case MSHIFTBETA_B: 
      SUCC1_OFFS =FLAGS_OFFS+CHARSIZE; 
      SUCC2_OFFS =SUCC1_OFFS+CODESIZE; 
      BOFF_OFFS  =SUCC2_OFFS+CODESIZE;  
      I_FREQ_OFFS=BOFF_OFFS+INTSIZE;
      L_FREQ_OFFS=CODESIZE; 
      I_FREQ_NUM=2;
      L_FREQ_NUM=1;

      ttype=tt;
      break;

    case MSHIFTBETA_I: 
      SUCC1_OFFS =FLAGS_OFFS+CHARSIZE; 
      SUCC2_OFFS =SUCC1_OFFS+CODESIZE; 
      BOFF_OFFS  =0;
      I_FREQ_OFFS=SUCC2_OFFS+CODESIZE;  
      L_FREQ_OFFS=CODESIZE; 
      I_FREQ_NUM=2;
      L_FREQ_NUM=1;

      ttype=tt;
      break;

    case SIMPLE_I:
      SUCC1_OFFS = 0;
      SUCC2_OFFS = 0;
      BOFF_OFFS  = 0;
      I_FREQ_OFFS= FLAGS_OFFS+CHARSIZE;
      L_FREQ_OFFS=CODESIZE; 
      I_FREQ_NUM=1;
      L_FREQ_NUM=1;

      ttype=tt;
      break;

    case SIMPLE_B: 

      SUCC1_OFFS  = 0;
      SUCC2_OFFS  = 0;
      BOFF_OFFS   = FLAGS_OFFS+CHARSIZE;
      I_FREQ_OFFS = BOFF_OFFS+INTSIZE;
      L_FREQ_OFFS = CODESIZE; 
      I_FREQ_NUM  = 1;
      L_FREQ_NUM  = 1;

      ttype=tt;
      break;

    case SHIFTBETA_I: 
      SUCC1_OFFS = FLAGS_OFFS+CHARSIZE;
      SUCC2_OFFS = 0;
      BOFF_OFFS  = 0;
      I_FREQ_OFFS= SUCC1_OFFS+CODESIZE;
      L_FREQ_OFFS=CODESIZE; 
      I_FREQ_NUM=1;
      L_FREQ_NUM=1;

      ttype=tt;
      break;

    case SHIFTBETA_B: 

      SUCC1_OFFS  = FLAGS_OFFS+CHARSIZE;
      SUCC2_OFFS  = 0;
      BOFF_OFFS   = SUCC1_OFFS+CODESIZE;
      I_FREQ_OFFS = BOFF_OFFS+INTSIZE;
      L_FREQ_OFFS = CODESIZE; 
      I_FREQ_NUM  = 1;
      L_FREQ_NUM  = 1;

      ttype=tt;
      break;

    case LEAFPROB: case FLEAFPROB:
      SUCC1_OFFS  = 0;
      SUCC2_OFFS  = 0;
      BOFF_OFFS   = 0;
      I_FREQ_OFFS = FLAGS_OFFS+CHARSIZE;
      I_FREQ_NUM  = 0;
      L_FREQ_NUM  = 1;

      ttype=tt;
      break;

    case LEAFPROB2:
      SUCC1_OFFS =0;
      SUCC2_OFFS =0;
      BOFF_OFFS  =0;
      I_FREQ_OFFS=FLAGS_OFFS+CHARSIZE;
      I_FREQ_NUM=0;
      L_FREQ_NUM=2;

      ttype=LEAFPROB;
      break;

    case LEAFPROB3:
      SUCC1_OFFS =0;
      SUCC2_OFFS =0;
      BOFF_OFFS  =0;
      I_FREQ_OFFS=FLAGS_OFFS+CHARSIZE;
      I_FREQ_NUM=0;
      L_FREQ_NUM=3;

      ttype=LEAFPROB;
      break;

    case LEAFPROB4:
      SUCC1_OFFS =0;
      SUCC2_OFFS =0;
      BOFF_OFFS  =0;
      I_FREQ_OFFS=FLAGS_OFFS+CHARSIZE;
      I_FREQ_NUM=0;
      L_FREQ_NUM=4;

      ttype=LEAFPROB;
      break;
        
    default:
      assert(tt==COUNT);
    }
    
    L_FREQ_OFFS=CODESIZE; 
  }

  int inodesize(int s){
    
    return I_FREQ_OFFS + I_FREQ_NUM * s;

  }
    
  int lnodesize(int s){
    return L_FREQ_OFFS + L_FREQ_NUM * s;
  }

};


class ngramtable:tabletype{

  node            tree; // ngram table root
  int           maxlev; // max storable n-gram
  NODETYPE   treeflags;
  char       info[100]; //information put in the header
  int       resolution; //max resolution for probabilities
  double         decay; //decay constant 
  
  storage*         mem; //memory storage class
  
  int*          memory; // memory load per level
  int*       occupancy; // memory occupied per level
  long long*     mentr; // multiple entries per level
  long long       card; //entries at maxlev

  int              idx[MAX_NGRAM+1];
 
  int           oov_code,oov_size,du_code, bo_code; //used by prob;

  int             backoff_state; //used by prob;

 public:

  int         corrcounts; //corrected counters flag
	
  dictionary     *dict; // dictionary

	// filtering dictionary: 
	// if the first word of the ngram does not belong to filterdict
	// do not insert the ngram
  dictionary     *filterdict;
	
	ngramtable(char* filename,int maxl,char* is,char *oovlex,
             char* filterdictfile,
             int googletable=0,
             int dstco=0,char* hmask=NULL,int inplen=0,
             TABLETYPE tt=FULL,int codesize=DEFCODESIZE);

	inline char* ngtype(char *str=NULL){if (str!=NULL) strcpy(info,str);return info;}

  ~ngramtable();
  void freetree(node nd);
  void stat(int level=4);

  inline long long totfreq(long long v=-1){
    return (v==-1?freq(tree,INODE):freq(tree,INODE,v));
  }

  inline long long btotfreq(long long v=-1){
    return (v==-1?getfreq(tree,treeflags,1):setfreq(tree,treeflags,v,1));
  }

  inline long long entries(int lev){
    return mentr[lev];
  }
  
  int maxlevel(){return maxlev;}
	
//  void savetxt(char *filename,int sz=0);
  void savetxt(char *filename,int sz=0,int googleformat=0);
  void loadtxt(char *filename,int googletable=0);
  

  void savebin(char *filename,int sz=0);
  void savebin(mfstream& out);
  void savebin(mfstream& out,node nd,NODETYPE ndt,int lev,int mlev);

  void loadbin(const char *filename);
  void loadbin(mfstream& inp);
  void loadbin(mfstream& inp,node nd,NODETYPE ndt,int lev);

  void loadbinold(char *filename);
  void loadbinold(mfstream& inp,node nd,NODETYPE ndt,int lev);

  void generate(char *filename);
  void generate_dstco(char *filename,int dstco);
  void generate_hmask(char *filename,char* hmask,int inplen=0);

  void augment(ngramtable* ngt); 
  
  int scan(ngram& ng,ACTION action=CONT,int maxlev=-1){
    return scan(tree,INODE,0,ng,action,maxlev);}
  
  int succscan(ngram& h,ngram& ng,ACTION action,int lev){
    //return scan(h.link,h.info,h.lev,ng,action,lev);
    return scan(h.link,h.info,lev-1,ng,action,lev);
  }

  double prob(ngram ng);

  int scan(node nd,NODETYPE ndt,int lev,ngram& ng,ACTION action=CONT,int maxl=-1); 

  void show();
  
  void *search(table *tb,NODETYPE ndt,int lev,int n,int sz,int *w,
	       ACTION action,char **found=(char **)NULL);

  int mybsearch(char *ar, int n, int size, unsigned char *key, int *idx);  

  int put(ngram& ng);
  int put(ngram& ng,node nd,NODETYPE ndt,int lev);

  inline int get(ngram& ng){ return get(ng,maxlev,maxlev); }
  int get(ngram& ng,int n,int lev);

  int comptbsize(int n);
  table *grow(table *tb,NODETYPE ndt,int lev,int n,int sz,NODETYPE oldndt=0);
  
	
	bool check_dictsize_bound();

	
  inline int putmem(char* ptr,int value,int offs,int size){
    assert(ptr!=NULL);
    for (int i=0;i<size;i++) 
      ptr[offs+i]=(value >> (8 * i)) & 0xff;
    return value;
  }

  inline int getmem(char* ptr,int* value,int offs,int size){
    assert(ptr!=NULL);
    *value=ptr[offs] & 0xff;
    for (int i=1;i<size;i++) 
      *value= *value | ( ( ptr[offs+i] & 0xff ) << (8 *i));
    return *value;
  }
  
  inline long putmem(char* ptr,long long value,int offs,int size){
    assert(ptr!=NULL);
    for (int i=0;i<size;i++) 
      ptr[offs+i]=(value >> (8 * i)) & 0xffLL;
    return value;
  }
  
  inline long getmem(char* ptr,long long* value,int offs,int size){
    assert(ptr!=NULL);
    *value=ptr[offs] & 0xff;
    for (int i=1;i<size;i++) 
      *value= *value | ( ( ptr[offs+i] & 0xffLL ) << (8 *i));
    return *value;
  }
  
  inline void tb2ngcpy(int* wordp,char* tablep,int n=1){
    for (int i=0;i<n;i++)
      getmem(tablep,&wordp[i],i*CODESIZE,CODESIZE);
  }

  inline void ng2tbcpy(char* tablep,int* wordp,int n=1){
    for (int i=0;i<n;i++)
      putmem(tablep,wordp[i],i*CODESIZE,CODESIZE);
  }

  inline int ngtbcmp(int* wordp,char* tablep,int n=1){
    int word;
    for (int i=0;i<n;i++){
      getmem(tablep,&word,i*CODESIZE,CODESIZE);
      if (wordp[i]!=word) return 1;
    }
    return 0;
  }

  inline int word(node nd,int value)
    {
      putmem(nd,value,WORD_OFFS,CODESIZE);
      return value;
    }

  inline int word(node nd)
    {
      int v;
      getmem(nd,&v,WORD_OFFS,CODESIZE);
      return v;
    }

  unsigned char mtflags(node nd,unsigned char value){
    return *(unsigned char *)(nd+FLAGS_OFFS)=value;
  }

  unsigned char mtflags(node nd){
    return *(unsigned char *)(nd+FLAGS_OFFS);
  }
  
  
  int update(ngram ng){
    
    if (!get(ng,ng.size,ng.size))
    {
      cerr << "cannot find " << ng << "\n";
      exit (1);
    }
    
    freq(ng.link,ng.pinfo,ng.freq);
    
    return 1;
  }
  
  long long freq(node nd,NODETYPE ndt,long long value)
  {
    int offs=(ndt & LNODE)?L_FREQ_OFFS:I_FREQ_OFFS;
    
    if (ndt & FREQ1)
      putmem(nd,value,offs,1);
    else if (ndt & FREQ2)
      putmem(nd,value,offs,2);
    else if (ndt & FREQ3)
      putmem(nd,value,offs,3);
    else if (ndt & FREQ4)
      putmem(nd,value,offs,4);
    else
      putmem(nd,value,offs,6);
    return value;
  }

  long long freq(node nd,NODETYPE ndt)
  {
    int offs=(ndt & LNODE)?L_FREQ_OFFS:I_FREQ_OFFS;
    long long value;
    
    if (ndt & FREQ1)
      getmem(nd,&value,offs,1);
    else if (ndt & FREQ2)
      getmem(nd,&value,offs,2);
    else if (ndt & FREQ3)
      getmem(nd,&value,offs,3);
    else if (ndt & FREQ4)
      getmem(nd,&value,offs,4);
    else 
      getmem(nd,&value,offs,6);
    
    return value;
  }
  

  long long setfreq(node nd,NODETYPE ndt,long long value,int index=0)
  {
    int offs=(ndt & LNODE)?L_FREQ_OFFS:I_FREQ_OFFS;
    
    if (ndt & FREQ1)
      putmem(nd,value,offs+index * 1,1);
    else if (ndt & FREQ2)
      putmem(nd,value,offs+index * 2,2);
    else if (ndt & FREQ3)
      putmem(nd,value,offs+index * 3,3);
    else if (ndt & FREQ4)
      putmem(nd,value,offs+index * 4,4);
    else
      putmem(nd,value,offs+index * 6,6);
    
    return value;
  }

  long long getfreq(node nd,NODETYPE ndt,int index=0)
  {
    int offs=(ndt & LNODE)?L_FREQ_OFFS:I_FREQ_OFFS;
    
    long long value;
    
    if (ndt & FREQ1)
      getmem(nd,&value,offs+ index * 1,1);
    else if (ndt & FREQ2)
      getmem(nd,&value,offs+ index * 2,2);
    else if (ndt & FREQ3)
      getmem(nd,&value,offs+ index * 3,3);
    else if (ndt & FREQ4)
      getmem(nd,&value,offs+ index * 4,4);
    else
      getmem(nd,&value,offs+ index * 6,6);
    
    return value;
  }


  double boff(node nd)
    {
      int value=0;
      getmem(nd,&value,BOFF_OFFS,INTSIZE);
      
      return double (value/(double)1000000000.0);
    }


  double myround(double x){
    long int i=(long int)(x);
    return (x-i)>0.500?i+1.0:(double)i;
  }
  
  int boff(node nd,double value)
    {
      int v=(int)myround(value * 1000000000.0);
      putmem(nd,v,BOFF_OFFS,INTSIZE);

      return 1;
    }

  int succ2(node nd,int value)
    {
      putmem(nd,value,SUCC2_OFFS,CODESIZE);
      return value;
    }

  int succ2(node nd)
    {
      int value=0;
      getmem(nd,&value,SUCC2_OFFS,CODESIZE);
      return value;
    }

  int succ1(node nd,int value)
    {
      putmem(nd,value,SUCC1_OFFS,CODESIZE);
      return value;
    }

  int succ1(node nd)
    {
      int value=0;
      getmem(nd,&value,SUCC1_OFFS,CODESIZE);
      return value;
    }
  
  int msucc(node nd,int value)
    {
      putmem(nd,value,MSUCC_OFFS,CODESIZE);
      return value;
    }

  int msucc(node nd)
    {
      int value;
      getmem(nd,&value,MSUCC_OFFS,CODESIZE);
      return value;
    }

  table mtable(node nd)
  {
    char v[PTRSIZE];;
    for (int i=0;i<PTRSIZE;i++) 
      v[i]=nd[MTAB_OFFS+i];
    
    return *(table *)v;
  }

  table mtable(node nd,table value)
  {
    char *v=(char *)&value;
    for (int i=0;i<PTRSIZE;i++) 
      nd[MTAB_OFFS+i]=v[i];
    return value;
  }
  
  int mtablesz(node nd)
  {
    if (mtflags(nd) & LNODE){
      if (mtflags(nd) & FREQ1)
        return lnodesize(1);
      else if (mtflags(nd) & FREQ2)
        return lnodesize(2);
      else if (mtflags(nd) & FREQ3)
        return lnodesize(3);
      else if (mtflags(nd) & FREQ4)
        return lnodesize(4);
      else
        return lnodesize(6);
    }
    else if (mtflags(nd) & INODE){
      if (mtflags(nd) & FREQ1)
        return inodesize(1);
      else if (mtflags(nd) & FREQ2)
        return inodesize(2);
      else if (mtflags(nd) & FREQ3)
        return inodesize(3);
      else if (mtflags(nd) & FREQ4)
        return inodesize(4);
      else
        return inodesize(6);
    }
    else{
      cerr << "node has wrong flags\n";
      exit(1);
    }
	}
  

  int bo_state(int value=-1){
    return (value==-1?backoff_state:backoff_state=value);
  }
  
	
};

#endif




