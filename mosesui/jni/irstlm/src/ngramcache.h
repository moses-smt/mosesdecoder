// $Id: ngramcache.h 3679 2010-10-13 09:10:01Z bertoldi $

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

#ifndef MF_NGRAMCACHE_H
#define MF_NGRAMCACHE_H

#include "mempool.h"
#include "htable.h"


#define NGRAMCACHE_t ngramcache

#define NGRAMCACHE_LOAD_FACTOR  0.5

typedef struct PROB_AND_STATE_ENTRY{
        double logpr;   //!< probability value of an ngram
        char* state;  //!< the largest suffix of an n-gram contained in the LM table.
        unsigned int statesize; //!< LM statesize of an ngram
        double bow;     //!< backoff weight
        int bol;        //!< backoff level
        PROB_AND_STATE_ENTRY(double lp=0.0, char* st=NULL, unsigned int stsz=0, double bw=0.0, int bl=0): logpr(lp), state(st), statesize(stsz), bow(bw), bol(bl) {}; //initializer
} prob_and_state_t;

void print(prob_and_state_t* pst,  std::ostream& out=std::cout);

class ngramcache{
private:

  static const bool debug=true;

  htable<int*>* ht;
  mempool *mp;
  int maxn;
  int ngsize;
  int infosize;
  int accesses;
  int hits;
  int entries;
  float load_factor; //!< ngramcache loading factor
void print(const int*);

public:
  ngramcache(int n,int size,int maxentries,float lf=NGRAMCACHE_LOAD_FACTOR);
  ~ngramcache();

  int cursize(){return entries;}
  int maxsize(){return maxn;}
  void reset(int n=0);
  char* get(const int* ngp,char*& info);
  char* get(const int* ngp,double& info);
  char* get(const int* ngp,prob_and_state_t& info);
  int add(const int* ngp,const char*& info);
  int add(const int* ngp,const double& info);
  int add(const int* ngp,const prob_and_state_t& info);
  int isfull(){return (entries >= maxn);}
  void stat();
  inline void used(){ stat(); };

  inline float set_load_factor(float value){ return load_factor=value; }
};

#endif

