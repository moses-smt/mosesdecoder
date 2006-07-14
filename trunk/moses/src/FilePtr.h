/* ---------------------------------------------------------------- */
/* Copyright 2005 (c) by RWTH Aachen - Lehrstuhl fuer Informatik VI */
/* Richard Zens                                                     */
/* ---------------------------------------------------------------- */
#ifndef FILEPTR_H_
#define FILEPTR_H_
#include "File.h"
#ifdef USECPFP
#include "CountedPointer.h"
#endif

template<typename T> class FilePtr {
public:
#ifdef USECPFP
  typedef CountedPointer<T> Ptr;
#else
  typedef T* Ptr;
#endif

private:
  FILE* f;
  off_t pos;
  mutable Ptr t;
public:
  FilePtr(FILE* f_=0,off_t p=0) : f(f_),pos(p),t(0) {}
  ~FilePtr() {}

  void set(FILE* f_,off_t p) {f=f_;pos=p;}

  void free() {
#ifdef USECPFP
    t.destroy();
#else
    delete t;  t=0;
#endif
  }

  T& operator* () {load();return *t;}
  Ptr operator->() {load();return t;}
  operator Ptr () {load();return t;}

  const T& operator* () const {load();return *t;}
  const Ptr operator->() const {load();return t;}
  operator const Ptr  () const {load();return t;}

  Ptr getPtr() {return t;}
  const Ptr getPtr() const {return t;}

  operator bool() const {return (f && pos!=InvalidOffT);}

  void load() const {
    if(t) return;
    if(f && pos!=InvalidOffT) {fSeek(f,pos); t=new T(f);}
  }
};
#endif
