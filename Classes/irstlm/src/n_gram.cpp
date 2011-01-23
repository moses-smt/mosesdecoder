// $Id: n_gram.cpp 3461 2010-08-27 10:17:34Z bertoldi $

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
#include <iomanip>
#include <cassert>
#include "mempool.h"
#include "htable.h"
#include "dictionary.h"
#include "n_gram.h"
#include "index.h"

using namespace std;

ngram::ngram(dictionary* d,int sz){
  dict=d;
  size=sz;
  succ=0;
  freq=0;
  info=0;
  pinfo=0;
  link=NULL;
  isym=-1;
  memset(word,0,sizeof(int)*MAX_NGRAM);
  memset(midx,0,sizeof(int)*MAX_NGRAM);
  memset(path,0,sizeof(char *)*MAX_NGRAM);
}

ngram::ngram(ngram& ng){
  size=ng.size;
  freq=ng.freq;
  succ=0;
  info=0;
  pinfo=0;
  link=NULL;
  isym=-1;
  dict=ng.dict;
  memcpy(word,ng.word,sizeof(int)*MAX_NGRAM);
  memcpy(midx,ng.word,sizeof(int)*MAX_NGRAM);

}

void ngram::trans (const ngram& ng){
  size=ng.size;
  freq=ng.freq;
  if (dict == ng.dict){
    info=ng.info;
    isym=ng.isym;      
    memcpy(word,ng.word,sizeof(int)*MAX_NGRAM);
    memcpy(midx,ng.midx,sizeof(int)*MAX_NGRAM);
  }
  else{
    info=0;
    memset(midx,0,sizeof(int)*MAX_NGRAM);
    isym=-1;
    for (int i=1;i<=size;i++)
      word[MAX_NGRAM-i]=dict->encode(ng.dict->decode(*ng.wordp(i)));
  }
}


void ngram::invert (const ngram& ng){
	size=ng.size;
	for (int i=1;i<=size;i++){
		*wordp(i)=*ng.wordp(size-i+1);
	}
}

void ngram::shift (){
	memmove((void *)&word[MAX_NGRAM-size+1],(void *)&word[MAX_NGRAM-size],(size-1) * sizeof(int));
	size--;
}


ifstream& operator>> ( ifstream& fi , ngram& ng){
  char w[MAX_WORD];
  memset(w,0,MAX_WORD);
  w[0]='\0';
  
  if (!(fi >> setw(MAX_WORD) >> w))
    return fi;
  
  if (strlen(w)==(MAX_WORD-1))
    cerr << "ngram: a too long word was read (" 
	 << w << ")\n";
    
  int c=ng.dict->encode(w);
  
  if (c == -1 ){
    cerr << "ngram: " << w << " is OOV \n";
    exit(1);
  }
  
  memcpy(ng.word,ng.word+1,(MAX_NGRAM-1)*sizeof(int));
  
  ng.word[MAX_NGRAM-1]=(int)c;
  ng.freq=1;
  
  if (ng.size<MAX_NGRAM) ng.size++;

  return fi;

}


int ngram::pushw(const char* w){
  
  assert(dict!=NULL);

  int c=dict->encode(w);

  if (c == -1 ){
    cerr << "ngram: " << w << " is OOV \n";
    exit(1);
  }
  
  pushc(c);

  return 1;

}

int ngram::pushc(int c){
 
  size++;
  if (size>MAX_NGRAM) size=MAX_NGRAM; 
  size_t len = size - 1; //i.e. if size==MAX_NGRAM, the farthest position is lost
  size_t src = MAX_NGRAM - len;
 
  memmove((void *)&word[src - 1],(void *)&word[src], len * sizeof(int));
  
/*
  int buff[MAX_NGRAM-1];
  memcpy(buff,word+1,(MAX_NGRAM-1)*sizeof(int));
  memcpy(word,buff,(MAX_NGRAM-1)*sizeof(int));
*/
  word[MAX_NGRAM-1]=c; // fill the most recent position

  return 1;

}

int ngram::pushc(int* codes, int codes_len){
//copy the first codes_len elements from codes into the actual ngram; sz must be smaller than MAX_NGRAM
//shift codes_len elements of the ngram backwards
  assert (codes_len <= MAX_NGRAM);

  size+=codes_len;

/*
std::cout << " codes_len:" << codes_len
        << " size:" << size
        << std::endl;
*/
  if (size>MAX_NGRAM) size=MAX_NGRAM;
  size_t len = size - codes_len;
  size_t src = MAX_NGRAM - len;

/*
std::cout << " codes_len:" << codes_len
 	<< " size:" << size
 	<< " len:" << len
 	<< " src:" << src
 	<< std::endl;
*/
  if (len > 0) memmove((void *)&word[src - codes_len],(void *)&word[src], len * sizeof(int));
//  memcpy((void *)&word[MAX_NGRAM - codes_len],(void*)&codes[MAX_NGRAM - codes_len],codes_len*sizeof(int));
  memcpy((void *)&word[MAX_NGRAM - codes_len],(void*)&codes[0],codes_len*sizeof(int));

  return 1;
}


istream& operator>> ( istream& fi , ngram& ng){
  char w[MAX_WORD];
  memset(w,0,MAX_WORD);
  w[0]='\0';
  
  assert(ng.dict != NULL);

  if (!(fi >> setw(MAX_WORD) >> w))
    return fi;
  
  if (strlen(w)==(MAX_WORD-1))
      cerr << "ngram: a too long word was read (" 
	   << w << ")\n";

  ng.pushw(w);

  ng.freq=1;
  
  return fi;

}

ofstream& operator<< (ofstream& fo,ngram& ng){

  assert(ng.dict != NULL);

  for (int i=ng.size;i>0;i--)
    fo << ng.dict->decode(ng.word[MAX_NGRAM-i]) << (i>1?" ":"");
  fo << "\t" << ng.freq; 
  return fo;
}

ostream& operator<< (ostream& fo,ngram& ng){

  assert(ng.dict != NULL);

  for (int i=ng.size;i>0;i--)
    fo << ng.dict->decode(ng.word[MAX_NGRAM-i]) << (i>1?" ":"");
  fo << "\t" << ng.freq; 
 
  return fo;
}

/*
main(int argc, char** argv){
  dictionary d(argv[1]);
  ifstream txt(argv[1]);
  ngram ng(&d);
  
  while (txt >> ng){
    cout << ng << "\n";
  }

  ngram ng2=ng;
  cerr << "copia l'ultimo =" << ng << "\n";
}
*/

