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


#include <stdexcept>
#include <assert.h>

#include "math.h"
#include "mempool.h"
#include "htable.h"

#include "ngramcache.h"

ngramcache::ngramcache(int n,int size,int maxentries){
      ngsize=n;
      infosize=size;
      maxn=maxentries;
      entries=0;
      ht=new htable(maxn * 2, ngsize * sizeof(int),INT,NULL); //load factor 2   
      mp=new mempool(ngsize * sizeof(int)+infosize,maxn/10); 
      accesses=0;
      hits=0;
    };
  
ngramcache::~ngramcache(){
    ht->stat();//ht->map();
    mp->stat();
    delete ht;delete mp;
};


void ngramcache::reset(){
    ht->stat();
    delete ht;delete mp;    
    ht=new htable(maxn * 2, ngsize * sizeof(int),INT,NULL); //load factor 2   
    mp=new mempool(ngsize * sizeof(int)+infosize,maxn/10); 
    entries=0;
  }


char* ngramcache::get(const int* ngp,char* info){       
    char *found;
   // cout << "ngramcache::get() ";
    //for (int i=0;i<ngsize;i++) cout << ngp[i] << " "; cout <<"\n"; 
  accesses++;
  if (found=ht->search((char *)ngp,HT_FIND)){
    if (info) memcpy(info,found+ngsize*sizeof(int),infosize);
    hits++;
  }; 
    return found;
  };
  

int ngramcache::add(const int* ngp,const char* info){
    
    char* entry=mp->alloc();
    memcpy(entry,(char*) ngp,sizeof(int) * ngsize);    
    memcpy(entry + ngsize * sizeof(int),(char *)info,infosize);
    char *found=ht->search((char *)entry,HT_ENTER);
    assert(found == entry); //false if key is already insided
    entries++;
    return 1;
  }
  
void ngramcache::stat(){
   cerr << "ngramcache stats: entries=" << entries << " acc=" << accesses << " hits=" << hits << "\n";
};

