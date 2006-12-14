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

// n-gram tables 
// by M. Federico
// Copyright Marcello Federico, ITC-irst, 1998

#ifndef MF_NGRAM_H
#define MF_NGRAM_H

#include <fstream>
#include <cassert>
#include "dictionary.h"

#ifdef MYMAXNGRAM
#define MAX_NGRAM MYMAXNGRAM
#else
#define MAX_NGRAM 20
#endif

class dictionary;

//typedef int code;

class ngram{
  int  word[MAX_NGRAM];  //encoded ngram
 public:
  dictionary *dict;      //dictionary
  char* link;            // ngram-tree pointer
  int  midx[MAX_NGRAM];  // ngram-tree scan pointer
  int    lev;            // ngram-tree level
  int   size;            // ngram size
  int   freq;            // ngram frequency or integer prob
  int   succ;            // number of successors
  int   bow;             // back-off weight 
  int   prob;            // probability
  
  unsigned char info;    // ngram-tree info flags
  unsigned char pinfo;   // ngram-tree parent info flags
  int  isym;             // last interruption symbol

  ngram(dictionary* d,int sz=0);
  ngram(ngram& ng);
  
  int *wordp()// n-gram pointer
    {return wordp(size);}; 
  int *wordp(int k) // n-gram pointer
    {return size>=k?&word[MAX_NGRAM-k]:0;}; 
  const int *wordp() const // n-gram pointer
    {return wordp(size);}; 
  const int *wordp(int k) const // n-gram pointer
    {return size>=k?&word[MAX_NGRAM-k]:0;}; 

  int shift(){
    for (int i=(MAX_NGRAM-1);i>0;i--){
      word[i]=word[i-1];
    }
    size--;
    return 1;
  }


  int containsWord(char* s,int lev){

    int c=dict->encode(s);
    if (c == -1) return 0;

    assert(lev <= size);
    for (int i=0;i<lev;i++){
      if (*wordp(size-i)== c) return 1;
    }
    return 0;
  }
    

  void trans(const ngram& ng);

  friend std::ifstream& operator>> (std::ifstream& fi,ngram& ng);
  friend std::ofstream& operator<< (std::ofstream& fi,ngram& ng);
  friend std::istream& operator>> (std::istream& fi,ngram& ng);
  friend std::ostream& operator<< (std::ostream& fi,ngram& ng);

  inline int ckhisto(int sz){
    
    for (int i=sz;i>1;i--) 
      if (*wordp(i)==dict->oovcode())
	return 0;
    return 1;
  }

  int pushc(int c);
  int pushw(char* w);

  //~ngram();



};

#endif



