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

#ifndef MF_DICTIONARY_H
#define MF_DICTIONARY_H

#include <cstring>
#include <iostream>

#define MAX_WORD 1000
#define LOAD_FACTOR  5

#ifndef GROWTH_STEP 
#define GROWTH_STEP 100000
#endif

#ifndef DICT_INITSIZE
#define DICT_INITSIZE 100000
#endif

//Begin of sentence symbol
#ifndef BOS_
#define BOS_ "<s>"
#endif


//End of sentence symbol
#ifndef EOS_ 
#define EOS_ "</s>"
#endif

//Out-Of-Vocabulary symbol
#ifndef OOV_ 
#define OOV_ "_unk_"
#endif

typedef struct{
  char *word;
  int  code;
  int  freq;
}dict_entry;

class strstack;
class htable;

class dictionary{
  strstack   *st;  //!< stack of strings
  dict_entry *tb;  //!< entry table
  htable    *htb;  //!< hash table
  int          n;  //!< number of entries
  int          N;  //!< total frequency
  int        lim;  //!< limit of entries
  int   oov_code;  //!< code assigned to oov words
  char*       is;  //!< interruption symbol list
  char       ifl;  //!< increment flag
  int        dubv; //!< dictionary size upper bound
  int in_oov_lex;  //!< flag
  int oov_lex_code; //!< dictionary
  char* oov_str;   //!< oov string

 public:

  friend class dictionary_iter;

  dictionary* oovlex; //<! additional dictionary 

  inline int dub(){return dubv;}
  inline int dub(int value){return (dubv=value);}

  inline char *OOV(){return (OOV_);} 
  inline char *BoS(){return (BOS_);}
  inline char *EoS(){return (EOS_);}

  inline int oovcode(int v=-1){return oov_code=(v>=0?v:oov_code);}
  
  inline char *intsymb(char* isymb=NULL){
    if (isymb==NULL) return is;
    if (is!=NULL) delete [] is;
    is=new char[strlen(isymb+1)];
    strcpy(is,isymb);
    return is=isymb;
  }

  inline int incflag(){return ifl;}
  inline int incflag(int v){return ifl=v;}
  inline int oovlexsize(){return oovlex?oovlex->n:0;}
  inline int inoovlex(){return in_oov_lex;}
  inline int oovlexcode(){return oov_lex_code;}
  

  int isprintable(char* w){
    char buffer[MAX_WORD];
    sprintf(buffer,"%s",w);
    return strcmp(w,buffer)==0;
  }

  inline void genoovcode(){
    int c=encode(OOV());
    std::cerr << "OOV code is "<< c << std::endl;
    oovcode(c);
  }
  
  inline dictionary* oovlexp(char *fname=NULL){
    if (fname==NULL) return oovlex;
    if (oovlex!=NULL) delete oovlex;
    oovlex=new dictionary(fname,DICT_INITSIZE);
    return oovlex;
  }

  inline int setoovrate(double oovrate){ 
    encode(OOV()); //be sure OOV code exists
    int oovfreq=(int)(oovrate * totfreq());
    std::cerr << "setting OOV rate to: " << oovrate << " -- freq= " << oovfreq << std::endl;
    return freq(oovcode(),oovfreq);
  }


  inline int incfreq(int code,int value){N+=value;return tb[code].freq+=value;}

  inline int multfreq(int code,double value){
    N+=(int)(value * tb[code].freq)-tb[code].freq;
    return tb[code].freq=(int)(value * tb[code].freq);
  }
  
  inline int freq(int code,int value=-1){
    if (value>=0){
      N+=value-tb[code].freq; 
      tb[code].freq=value;
    }
    return tb[code].freq;
  }

  inline int totfreq(){return N;}

  void grow();
  //dictionary(int size=400,char* isym=NULL,char* oovlex=NULL);
  dictionary(char *filename=NULL,int size=DICT_INITSIZE,char* isymb=NULL,char* oovlex=NULL);
  dictionary(dictionary* d);

  ~dictionary();
  void generate(char *filename);
  void load(char *filename);
  void save(char *filename,int freqflag=0);
  void load(std::istream& fd);
  void save(std::ostream& fd);

  int size(){return n;};
  int getcode(const char *w);
  int encode(const char *w);
  char *decode(int c);
  void stat();

  void cleanfreq(){
    for (int i=0;i<n;tb[i++].freq=0); 
    N=0;
  }

};

class dictionary_iter {
 public:
  dictionary_iter(dictionary *dict);
  dict_entry* next();
 private:
  dictionary* m_dict;
};

#endif

