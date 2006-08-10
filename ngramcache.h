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

class ngramcache{
private:
  htable* ht;  
  mempool *mp;
  int maxn;
  int ngsize;
  int infosize;
  int accesses;
  int hits;
  int entries;

public:
    
  ngramcache(int n,int size,int maxentries);  
  ~ngramcache();  
  void reset();  
  char* get(const int* ngp,char* info=NULL);  
  int add(const int* ngp,const char* info);  
  int isfull(){return (entries >= maxn);};  
  void stat();
};

#endif