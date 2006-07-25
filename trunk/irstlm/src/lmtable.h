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

#ifndef MF_LMTABLE_H
#define MF_LMTABLE_H

#include "ngram.h"

#define LMTMAXLEV  11

#ifndef  LMTCODESIZE
#define  LMTCODESIZE  (int)3
#endif

#define SHORTSIZE (int)2
#define PTRSIZE   (int)sizeof(char *)
#define INTSIZE   (int)4
#define CHARSIZE  (int)1

#define PROBSIZE  (int)4 //use float  
#define QPROBSIZE (int)1 
#define BOUNDSIZE (int)4

#define UNIGRAM_RESOLUTION 10000000.0

typedef enum {INTERNAL,QINTERNAL,LEAF,QLEAF} LMT_TYPE;
typedef char* node;

typedef enum {LMT_FIND,    //!< search: find an entry
	      LMT_ENTER,   //!< search: enter an entry 
	      LMT_INIT,    //!< scan: start scan
	      LMT_CONT     //!< scan: continue scan
} LMT_ACTION;


class lmtable{

  char*      table[LMTMAXLEV]; //storage of all levels
  LMT_TYPE tbltype[LMTMAXLEV]; //table type for each levels
  int      cursize[LMTMAXLEV]; //current size of levels
  int      maxsize[LMTMAXLEV]; //current size of levels
  int*    startpos[LMTMAXLEV]; //support vector to store start positions

  int               maxlev; //max level of table
  char           info[100]; //information put in the header

  //probability quantization
  bool      isQtable;
  
  int       NumCenters[LMTMAXLEV];
  float*    Pcenters[LMTMAXLEV];
  float*    Bcenters[LMTMAXLEV];
  
  int     lmt_oov_code;
  int     lmt_oov_size;
  int    backoff_state; 


 public:

  dictionary     *dict; // dictionary

  lmtable(std::istream& in);

  ~lmtable(){
    for (int i=1;i<=maxlev;i++){
      delete [] table[i];
      if (isQtable){
	delete [] Pcenters[i];
	if (i<maxlev) delete [] Bcenters[i];
      }
    }
  }
	
	void configure(int n,bool quantized){
		maxlev=n;
		if (n==1)
			tbltype[1]=(quantized?QLEAF:LEAF);
		else{
			for (int i=1;i<n;i++) tbltype[i]=(quantized?QINTERNAL:INTERNAL);
			tbltype[n]=(quantized?QLEAF:LEAF);
			}
	};
	
  int maxlevel(){return maxlev;};

  void savetxt(const char *filename);
  void savebin(const char *filename);
  void dumplm(std::ostream& out,ngram ng, int ilev, int elev, int ipos,int epos);

  void loadtxt(std::istream& in, const char* header);
  void loadbin(std::istream& in, const char* header);

  void loadcenters(std::istream& inp,int Order);

  double prob(ngram ng); 

  void *search(char *tb,LMT_TYPE ndt,int lev,int n,int sz,int *w,
	       LMT_ACTION action,char **found=(char **)NULL);

  int mybsearch(char *ar, int n, int size, unsigned char *key, int *idx);   
  
  int add(ngram& ng,int prob,int bow);
  void checkbounds(int level);
  
  int get(ngram& ng){return get(ng,ng.size,ng.size);}
  int get(ngram& ng,int n,int lev);

  int succscan(ngram& h,ngram& ng,LMT_ACTION action,int lev);
	const char *maxsuffptr(ngram ong);
  inline int putmem(char* ptr,int value,int offs,int size){
    assert(ptr!=NULL);
    for (int i=0;i<size;i++)
      ptr[offs+i]=(value >> (8 * i)) & 0xff;
    return value;
  };

  inline int getmem(char* ptr,int* value,int offs,int size){
    assert(ptr!=NULL);
    *value=ptr[offs] & 0xff;
    for (int i=1;i<size;i++)
      *value= *value | ( ( ptr[offs+i] & 0xff ) << (8 *i));
    return *value;
  };
  

  int bo_state(int value=-1){ 
    return (value==-1?backoff_state:backoff_state=value); 
  };


  int nodesize(LMT_TYPE ndt){
    switch (ndt){
    case INTERNAL:
      return LMTCODESIZE + PROBSIZE + PROBSIZE + BOUNDSIZE;
    case QINTERNAL:
      return LMTCODESIZE + QPROBSIZE + QPROBSIZE + BOUNDSIZE;
    case QLEAF:
      return LMTCODESIZE + QPROBSIZE;      
    case LEAF:
      return LMTCODESIZE + PROBSIZE;      
    default:
      assert(0);
      return 0;
    }
  }  

 inline int word(node nd,int value=-1)
    {
      int offset=0;

      if (value==-1)
	getmem(nd,&value,offset,LMTCODESIZE);
      else
	putmem(nd,value,offset,LMTCODESIZE);

      return value;
    };

 inline int prob(node nd,LMT_TYPE ndt, int value=-1)
    {
      int offs=LMTCODESIZE;
      int size=(ndt==QINTERNAL || ndt==QLEAF?QPROBSIZE:PROBSIZE);

      if (value==-1)
	getmem(nd,&value,offs,size);
      else
	putmem(nd,value,offs,size);

      return value;
    };


 inline int bow(node nd,LMT_TYPE ndt, int value=-1)
    {
      assert(ndt==INTERNAL || ndt==QINTERNAL);
      int size=(ndt==QINTERNAL?QPROBSIZE:PROBSIZE);
      int offs=LMTCODESIZE+size;
      
      if (value==-1)
	getmem(nd,&value,offs,size);
      else
	putmem(nd,value,offs,size);

      return value;
    };

 inline int bound(node nd,LMT_TYPE ndt, int value=-1)
    {
      assert(ndt==INTERNAL || ndt==QINTERNAL);
      int offs=LMTCODESIZE+2*(ndt==QINTERNAL?QPROBSIZE:PROBSIZE);
      
      if (value==-1)
	getmem(nd,&value,offs,BOUNDSIZE);
      else
	putmem(nd,value,offs,BOUNDSIZE);

      return value;
    };

 void stat(int lev=0);
 
};

#endif




