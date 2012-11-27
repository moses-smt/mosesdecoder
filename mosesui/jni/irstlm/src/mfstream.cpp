// $Id: mfstream.cpp 294 2009-08-19 09:57:27Z mfederico $

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

#include <iostream>
#include <fstream>
#include <streambuf>
#include <cstdio>
#include "mfstream.h"

using namespace std;

void mfstream::open(const char *name,openmode mode){
  
  char cmode[10];

  if (strchr(name,' ')!=0){ 
    if (mode & ios::in)
      strcpy(cmode,"r");
    else
      if (mode & ios::out) 
	strcpy(cmode,"w");
      else
	if (mode & ios::app) 
	  strcpy(cmode,"a");    
	else{
	  cerr << "cannot open file\n";
	  exit(1);
	}
    _cmd=1;
    strcpy(_cmdname,name);
    _FILE=popen(name,cmode);
    buf=new fdbuf(fileno(_FILE));
    iostream::rdbuf((streambuf*) buf);
  }
  else{
    _cmd=0;
    fstream::open(name,mode);
  }
  
}


void mfstream::close(){
  if (_cmd==1){
    pclose(_FILE);
    delete buf;
  }
  else {
    fstream::clear();
    fstream::close();
  }
  _cmd=2;
}



int mfstream::swapbytes(char *p, int sz, int n)
{
  char    c,
    *l,
    *h;
  
  if((n<1) ||(sz<2)) return 0;
  for(; n--; p+=sz) for(h=(l=p)+sz; --h>l; l++) { c=*h; *h=*l; *l=c; }
  return 0;

};


mfstream& mfstream::iwritex(streampos loc,void *ptr,int size,int n)
{
  streampos pos=tellp();
   
  seekp(loc);
   
  writex(ptr,size,n);
  
  seekp(pos);
  
  return *this;
   
}


mfstream& mfstream::readx(void *p, int sz,int n)
{
  if(!read((char *)p, sz * n)) return *this;
  
  if(*(short *)"AB"==0x4241){
    swapbytes((char*)p, sz,n);
  }

  return *this;
}

mfstream& mfstream::writex(void *p, int sz,int n)
{
  if(*(short *)"AB"==0x4241){
    swapbytes((char*)p, sz,n);
  }
  
  write((char *)p, sz * n);
  
  if(*(short *)"AB"==0x4241) swapbytes((char*)p, sz,n);
  
  return *this;
}




/*
int main()
{
  
  char word[1000];

  mfstream inp("cat pp",ios::in); 
  mfbstream outp("aa",ios::out,100);

  while (inp >> word){
    outp << word << "\n"; 
    cout << word << "\n";
  }

  
}

*/
