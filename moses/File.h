// $Id$

/* ---------------------------------------------------------------- */
/* Copyright 2005 (c) by RWTH Aachen - Lehrstuhl fuer Informatik VI */
/* Richard Zens                                                     */
/* ---------------------------------------------------------------- */
#ifndef moses_File_h
#define moses_File_h

#include <cstdio>
#include <iostream>
#include <vector>
#include "util/exception.hh"
#include "UserMessage.h"
#include "TypeDef.h"
#include "Util.h"

namespace Moses
{

#ifdef WIN32
#define OFF_T __int64
#define FTELLO(file) _ftelli64(file)
#define FSEEKO(file, offset, origin) _fseeki64(file, offset, origin)

#else
#define OFF_T off_t
#define FTELLO(f) ftello(f)
#define FSEEKO(file, offset, origin) fseeko(file, offset, origin)
#endif

static const OFF_T InvalidOffT=-1;

//  WARNING:
//    these functions work only for bitwise read/write-able types

template<typename T> inline size_t fWrite(FILE* f,const T& t)
{
  if(fwrite(&t,sizeof(t),1,f)!=1) {
    TRACE_ERR("ERROR:: fwrite!\n");
    abort();
  }
  return sizeof(t);
}

template<typename T> inline void fRead(FILE* f,T& t)
{
  if(fread(&t,sizeof(t),1,f)!=1) {
    TRACE_ERR("ERROR: fread!\n");
    abort();
  }
}

template<typename T> inline size_t fWrite(FILE* f,const T* b,const T* e)
{
  UINT32 s=std::distance(b,e);
  size_t rv=fWrite(f,s);
  if(fwrite(b,sizeof(T),s,f)!=s) {
    TRACE_ERR("ERROR: fwrite!\n");
    abort();
  }
  return rv+sizeof(T)*s;
}

template<typename T> inline size_t fWrite(FILE* f,const T b,const T e)
{
  UINT32 s=std::distance(b,e);
  size_t rv=fWrite(f,s);
  if(fwrite(&(*b),sizeof(T),s,f)!=s) {
    TRACE_ERR("ERROR: fwrite!\n");
    abort();
  }
  return rv+sizeof(T)*s;
}

template<typename C> inline size_t fWriteVector(FILE* f,const C& v)
{
  UINT32 s=v.size();
  size_t rv=fWrite(f,s);
  if(fwrite(&v[0],sizeof(typename C::value_type),s,f)!=s) {
    TRACE_ERR("ERROR: fwrite!\n");
    abort();
  }
  return rv+sizeof(typename C::value_type)*s;
}

template<typename C> inline void fReadVector(FILE* f, C& v)
{
  UINT32 s;
  fRead(f,s);
  v.resize(s);
  size_t r=fread(&(*v.begin()),sizeof(typename C::value_type),s,f);
  if(r!=s) {
    TRACE_ERR("ERROR: freadVec! "<<r<<" "<<s<<"\n");
    abort();
  }
}

inline size_t fWriteString(FILE* f,const char* e, UINT32 s)
{
  size_t rv=fWrite(f,s);
  if(fwrite(e,sizeof(char),s,f)!=s) {
    TRACE_ERR("ERROR:: fwrite!\n");
    abort();
  }
  return rv+sizeof(char)*s;
}

inline void fReadString(FILE* f,std::string& e)
{
  UINT32 s;
  fRead(f,s);
  char* a=new char[s+1];
  if(fread(a,sizeof(char),s,f)!=s) {
    TRACE_ERR("ERROR: fread!\n");
    abort();
  }
  a[s]='\0';
  e.assign(a);
  delete[](a);
}

inline size_t fWriteStringVector(FILE* f,const std::vector<std::string>& v)
{
  UINT32 s=v.size();
  size_t totrv=fWrite(f,s);
  for (size_t i=0; i<s; i++) {
    totrv+=fWriteString(f,v.at(i).c_str(),v.at(i).size());
  }
  return totrv;
}

inline void fReadStringVector(FILE* f, std::vector<std::string>& v)
{
  UINT32 s;
  fRead(f,s);
  v.resize(s);

  for (size_t i=0; i<s; i++) {
    fReadString(f,v.at(i));
  }
}

inline OFF_T fTell(FILE* f)
{
  return FTELLO(f);
}

inline void fSeek(FILE* f,OFF_T o)
{
  if(FSEEKO(f,o,SEEK_SET)<0) {
    TRACE_ERR("ERROR: could not fseeko position "<<o<<"\n");
    if(o==InvalidOffT) TRACE_ERR("You tried to seek for 'InvalidOffT'!\n");
    abort();
  }
}

inline FILE* fOpen(const char* fn,const char* m)
{
  if(FILE* f=fopen(fn,m))
    return f;
  else {
    UserMessage::Add(std::string("ERROR: could not open file ") + fn + " with mode " + m + "\n");
    UTIL_THROW(util::Exception, "Couldn't open file " << fn);
    return NULL;
  }
}
inline void fClose(FILE* f)
{
  fclose(f); // for consistent function names only
}

}

#endif
