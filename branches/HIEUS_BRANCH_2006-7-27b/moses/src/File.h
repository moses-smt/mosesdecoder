/* ---------------------------------------------------------------- */
/* Copyright 2005 (c) by RWTH Aachen - Lehrstuhl fuer Informatik VI */
/* Richard Zens                                                     */
/* ---------------------------------------------------------------- */
// $Id$
#ifndef FILE_H_
#define FILE_H_
#include <cstdio>
#include <vector>

static const off_t InvalidOffT=-1;

//  WARNING:
//    these functions work only for bitwise read/write-able types

template<typename T> inline size_t fWrite(FILE* f,const T& t) {
  if(fwrite(&t,sizeof(t),1,f)!=1) {
    std::cerr<<"ERROR:: fwrite!\n";abort();}
  return sizeof(t);
}

template<typename T> inline void fRead(FILE* f,T& t)  {
  if(fread(&t,sizeof(t),1,f)!=1) {std::cerr<<"ERROR: fread!\n";abort();}
}

template<typename T> inline size_t fWrite(FILE* f,const T* b,const T* e) {
  unsigned s=e-b;size_t rv=fWrite(f,s);
  if(fwrite(b,sizeof(T),s,f)!=s) {std::cerr<<"ERROR: fwrite!\n";abort();}
  return rv+sizeof(T)*s;
}

template<typename T> inline size_t fWrite(FILE* f,const T b,const T e) {
  unsigned s=std::distance(b,e);size_t rv=fWrite(f,s);
  if(fwrite(&(*b),sizeof(T),s,f)!=s) {std::cerr<<"ERROR: fwrite!\n";abort();}
  return rv+sizeof(T)*s;
}

template<typename C> inline size_t fWriteVector(FILE* f,const C& v) {
  unsigned s=v.size();
  size_t rv=fWrite(f,s);
  if(fwrite(&v[0],sizeof(typename C::value_type),s,f)!=s) {std::cerr<<"ERROR: fwrite!\n";abort();}
  return rv+sizeof(typename C::value_type)*s;
}

template<typename C> inline void fReadVector(FILE* f, C& v) {
  unsigned s;fRead(f,s);v.resize(s);
  unsigned r=fread(&(*v.begin()),sizeof(typename C::value_type),s,f);
  if(r!=s) {
    std::cerr<<"ERROR: freadVec! "<<r<<" "<<s<<"\n";abort();}
}

inline off_t fTell(FILE* f) {return ftello(f);}

inline void fSeek(FILE* f,off_t o) {
  if(fseeko(f,o,SEEK_SET)<0) {
    std::cerr<<"ERROR: could not fseeko position "<<o<<"\n";
    if(o==InvalidOffT) std::cerr<<"You tried to seek for 'InvalidOffT'!\n";
    abort();
  }
}

inline FILE* fOpen(const char* fn,const char* m) {
  if(FILE* f=fopen(fn,m)) return f; else {
    std::cerr<<"ERROR: could not open file "<<fn<<" with mode "<<m<<"\n";
    abort();}
}
inline void fClose(FILE* f) {fclose(f);} // for consistent function names only

#endif

