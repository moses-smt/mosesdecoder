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

#include <iomanip>
#include <iostream>
#include <fstream>
#include "mempool.h"
#include "htable.h"
#include "dictionary.h"


dictionary::dictionary(char *filename,int size,char* isymb,char* oovlexfile){

  // unitialized memory 
  if (oovlexfile!=NULL)
    oovlex=new dictionary(oovlexfile,size,isymb,NULL);
  else
    oovlex=NULL;

  htb = new htable(size/LOAD_FACTOR);
  tb  = new dict_entry[size]; 
  st  = new strstack(size * 10);

  for (int i=0;i<size;i++) tb[i].freq=0;
  
  is=NULL;
  intsymb(isymb);

  oov_code = -1;
  in_oov_lex=0;
  n  = 0;
  N  =  0;
  dubv = 0; 
  lim = size;
  int c=0; // counter
  ifl=0;  //increment flag

  if (filename==NULL) return;

  std::ifstream inp(filename,ios::in);
  
  if (!inp){
    cerr << "cannot open " << filename << "\n";
    exit(1);
  }
  
  char buffer[100];
  
  inp >> setw(100) >> buffer;
  
  inp.close();
  
  if ((strncmp(buffer,"dict",4)==0) ||
      (strncmp(buffer,"DICT",4)==0))
    load(filename);
  else
    generate(filename);
  
  cerr << "loaded \n";


}



void dictionary::generate(char *filename){

  char buffer[MAX_WORD];
  char *addr;
  int k,c;

  ifstream inp(filename,ios::in);
  
  if (!inp){
    cerr << "cannot open " << filename << "\n";
    exit(1);
  }

  cerr << "dict:";
  
  ifl=1; k=0;
  while (inp >> setw(MAX_WORD) >> buffer){
    
    if (strlen(buffer)==(MAX_WORD-1)){
      cerr << "dictionary: a too long word was read (" 
	   << buffer << ")\n";
    };
    
    
    if (strlen(buffer)==0){
      cerr << "zero lenght word!\n";
      continue;
    }

    //if (is && (strlen(buffer)==1) && !index(is,buffer[0]))  
    if (is && (strlen(buffer)==1) && (index(is,buffer[0])!=NULL))  
      continue; //skip over the interruption symbol

    incfreq(encode(buffer),1);

    if (!(++k % 1000000)) cerr << ".";
  }
  ifl=0;
  cerr << "\n";

  inp.close();

}

void dictionary::load(char* filename){
  char header[100];
  char buffer[MAX_WORD];
  char *addr;
  int freqflag=0;

  ifstream inp(filename,ios::in);
  
  if (!inp){
    cerr << "\ncannot open " << filename << "\n";
    exit(1);
  }

  cerr << "dict:";

  inp.getline(header,100);
  if (strncmp(header,"DICT",4)==0)
    freqflag=1;
  else 
    if (strncmp(header,"dict",4)!=0){
      cerr << "\ndictionary file " << filename << " has a wrong header\n";
      exit(1);
    }
      

  while (inp >> setw(MAX_WORD) >> buffer){
    
    if (strlen(buffer)==(MAX_WORD-1)){
      cerr << "\ndictionary: a too long word was read (" 
	   << buffer << ")\n";
    };
    
    tb[n].word=st->push(buffer);
    tb[n].code=n;

    if (freqflag) 
      inp >> tb[n].freq;
    else
      tb[n].freq=0;

    if (addr=htb->search((char  *)&tb[n].word,HT_ENTER))
      if (addr!=(char *)&tb[n].word){
	cerr << "dictionary::loadtxt wrong entry was found (" 
	     <<  buffer << ") in position " << n << "\n";
	exit(1);
      }

    N+=tb[n].freq;

    if (strcmp(buffer,OOV())==0) oov_code=n;

    if (++n==lim) grow();
      
  }
  
  inp.close();
}


void dictionary::load(std::istream& inp){
  
  char buffer[MAX_WORD];
  char *addr;
  int size;

  inp >> size;

  for (int i=0;i<size;i++){
    
    inp >> buffer;
    
    tb[n].word=st->push(buffer);
    tb[n].code=n;
    inp >> tb[n].freq;
    N+=tb[n].freq;

    if (addr=htb->search((char  *)&tb[n].word,HT_ENTER))
      if (addr!=(char *)&tb[n].word){
	cerr << "dictionary::loadtxt wrong entry was found (" 
	     <<  buffer << ") in position " << n << "\n";
	exit(1);
      }
    
    if (strcmp(tb[n].word,OOV())==0)
      oov_code=n;

    if (++n==lim) grow();
  }
  inp.getline(buffer,MAX_WORD-1);
}

void dictionary::save(std::ostream& out){
  out << n << "\n";
  for (int i=0;i<n;i++)
    out << tb[i].word << " " << tb[i].freq << "\n";
}


int cmpdictentry(const void *a,const void *b){
  dict_entry *ae=(dict_entry *)a;
  dict_entry *be=(dict_entry *)b;
  return be->freq-ae->freq;
}

dictionary::dictionary(dictionary* d){
  
  //transfer values
  
  n=d->n;        //total entries
  N=d->N;        //total frequency
  lim=d->lim;    //limit of entries
  oov_code=-1;   //code od oov must be re-defined
  ifl=0;         //increment flag=0;
  dubv=d->dubv;  //dictionary upperbound transferred
  in_oov_lex=0;  //does not copy oovlex;
  
    
  //creates a sorted copy of the table
  
  tb  = new dict_entry[lim]; 
  htb = new htable(lim/LOAD_FACTOR);
  st  = new strstack(lim * 10);

  for (int i=0;i<n;i++){
    tb[i].code=d->tb[i].code;
    tb[i].freq=d->tb[i].freq;
    tb[i].word=st->push(d->tb[i].word);
  }
  
  //sort all entries according to frequency
  cerr << "sorting dictionary ...";
  qsort(tb,n,sizeof(dict_entry),cmpdictentry);
  cerr << "done\n";

  for (int i=0;i<n;i++){
  
    //eventually re-assign oov code
    if (d->oov_code==tb[i].code) oov_code=i;

    tb[i].code=i;
    htb->search((char  *)&tb[i].word,HT_ENTER);
  }

};



dictionary::~dictionary(){
  delete htb;
  delete st;
  delete [] tb;
}

void dictionary::stat(){
  cout << "dictionary class statistics\n";
  cout << "size " << n
       << " used memory " 
       << (lim * sizeof(int) + 
	   htb->used() + 
	   st->used())/1024 << " Kb\n";
}

void dictionary::grow(){
  
  delete htb;  

  cerr << "+\b";

  dict_entry *tb2=new dict_entry[lim+GROWTH_STEP];
  
  memcpy(tb2,tb,sizeof(dict_entry) * lim );
  
  delete [] tb; tb=tb2;
  
  htb=new htable((lim+GROWTH_STEP)/LOAD_FACTOR);
  
  for (int i=0;i<lim;i++)
    
    htb->search((char *)&tb[i].word,HT_ENTER);

  for (int i=lim;i<lim+GROWTH_STEP;i++) tb[i].freq=0;
  
  lim+=GROWTH_STEP;
  

}

void dictionary::save(char *filename,int freqflag){
  
  std::ofstream out(filename,ios::out);

  if (!out){
    cerr << "cannot open " << filename << "\n";
  } 
  
  // header
  if (freqflag)
    out << "DICTIONARY 0 " << n << "\n";
  else
    out << "dictionary 0 " << n << "\n";

  for (int i=0;i<n;i++){
    out << tb[i].word;
    if (freqflag)
      out << " " << tb[i].freq;
    out << "\n";
  }

  out.close();
}


int dictionary::getcode(const char *w){
  dict_entry* ptr=(dict_entry *)htb->search((char *)&w,HT_FIND);
  if (ptr==NULL) return -1;
  return ptr->code;
}

int dictionary::encode(const char *w){
  
  if (strlen(w)==0)
    {
      cerr << "0";
      w=OOV();
    }

  dict_entry* ptr=(dict_entry *)htb->search((char *)&w,HT_FIND);
  if (ptr != 0)
    return ptr->code;
  else{
    if (!ifl){ //do not extend dictionary
      if (oov_code==-1){
	cerr << "starting to use OOV words [" << w << "]\n";
	tb[n].word=st->push(OOV());
	htb->search((char  *)&tb[n].word,HT_ENTER);
	tb[n].code=n;
	tb[n].freq=0;
	oov_code=n;
	if (++n==lim) grow();
      }
      dict_entry* oovptr;
      if (oovlex && (oovptr=(dict_entry *)oovlex->htb->search((char *)&w,HT_FIND))){
	in_oov_lex=1;
	oov_lex_code=oovptr->code;
      }
      else
	in_oov_lex=0;
      
      return encode(OOV()); 
    }
    else{ //extend dictionary
      tb[n].word=st->push((char *)w);
      htb->search((char  *)&tb[n].word,HT_ENTER);
      tb[n].code=n;
      tb[n].freq=0;
      if (++n==lim) grow();
      return n-1;
    }
  }
}


char *dictionary::decode(int c){
  if (c>=0 && c < n)
    return tb[c].word;
  else{
    cerr << "decode: code out of boundary\n";
    return OOV();
  }
}


dictionary_iter::dictionary_iter(dictionary *dict) : m_dict(dict) {
  m_dict->htb->scan(HT_INIT);
}

dict_entry* dictionary_iter::next() {
  return (dict_entry*)m_dict->htb->scan(HT_CONT);
}





/*
main(int argc,char **argv){
  dictionary d(argv[1],40000);
  d.stat();
  cout << "ROMA" << d.decode(0) << "\n";
  cout << "ROMA:" << d.encode("ROMA") << "\n";
  d.save(argv[2]);
}
*/
