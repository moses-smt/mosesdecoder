// $Id$

/* ---------------------------------------------------------------- */
/* Copyright 2005 (c) by RWTH Aachen - Lehrstuhl fuer Informatik VI */
/* Richard Zens                                                     */
/* ---------------------------------------------------------------- */

#ifndef moses_ObjectPool_h
#define moses_ObjectPool_h

#include <vector>
#include <deque>
#include <string>
#include <iostream>
#include <iterator>
#include "Util.h"

/***
 * template class for pool of objects
 * - useful if many small objects are frequently created and destroyed
 * - allocates memory for N objects at a time
 * - separates memory allocation from constructor/destructor calls
 * - prevents memory leaks
 */
template<typename T> class ObjectPool
{
public:
  typedef T Object;
private:
  std::string name;
  size_t idx,dIdx,N;
  std::vector<Object*> data;
  std::vector<size_t> dataSize;
  std::deque<Object*> freeObj;
  int mode;
public:
  static const int cleanUpOnDestruction=1;
  static const int hasTrivialDestructor=2;

  // constructor arguments:
  //   N: initial number of objects to allocate memory at a time
  //   m & cleanUpOnDestruction = clean up objects in destructor
  //   m & hasTrivialDestructor = the object type has a trivial destructor,
  //            i.e. no sub-object uses dynamically allocated memory
  //            note: not equivalent to empty destructor
  //         -> more efficient (destructor calls can be omitted),
  //            note: looks like memory leak, but is not
  ObjectPool(std::string name_="T",size_t N_=100000,int m=cleanUpOnDestruction)
    : name(name_),idx(0),dIdx(0),N(N_),mode(m) {
    allocate();
  }

  // main accesss functions:
  // get pointer to object via default or copy constructor
  Object* get() {
    return new (getPtr()) Object;
  }
  Object* get(const Object& x) {
    return new (getPtr()) Object(x);
  }

  // get pointer to uninitialized memory,
  // WARNING: use only if you know what you are doing !
  // useful for non-default constructors, you have to use placement new
  Object* getPtr() {
    if(freeObj.size()) {
      Object* rv=freeObj.back();
      freeObj.pop_back();
      rv->~Object();
      return rv;
    }
    if(idx==dataSize[dIdx]) {
      idx=0;
      if(++dIdx==data.size()) allocate();
    }
    return data[dIdx]+idx++;
  }

  // return object(s) to pool for reuse
  // note: objects are not destroyed here, but in 'getPtr'/'destroyObjects',
  //       otherwise 'destroyObjects' would have to check the freeObj-stack
  //       before each destructor call
  void freeObject(Object* x) {
    freeObj.push_back(x);
  }
  template<class fwiter> void freeObjects(fwiter b,fwiter e) {
    for(; b!=e; ++b) this->free(*b);
  }

  // destroy all objects, but do not free memory
  void reset() {
    destroyObjects();
    idx=0;
    dIdx=0;
    freeObj.clear();
  }
  // destroy all objects and free memory
  void cleanUp() {
    reset();
    for(size_t i=0; i<data.size(); ++i) free(data[i]);
    data.clear();
    dataSize.clear();
  }

  ~ObjectPool() {
    if(mode & cleanUpOnDestruction) cleanUp();
  }

  void printInfo(std::ostream& out) const {
    out<<"OPOOL ("<<name<<") info: "<<data.size()<<" "<<dataSize.size()<<" "
       <<freeObj.size()<<"\n"<<idx<<" "<<dIdx<<" "<<N<<"\n";
    std::copy(dataSize.begin(),dataSize.end(),
              std::ostream_iterator<size_t>(out," "));
    out<<"\n\n";
  }


private:
  void destroyObjects() {
    if(mode & hasTrivialDestructor) return;
    for(size_t i=0; i<=dIdx; ++i) {
      size_t lastJ= (i<dIdx ? dataSize[i] : idx);
      for(size_t j=0; j<lastJ; ++j) (data[i]+j)->~Object();
    }
  }
  // allocate memory for a N objects, for follow-up allocations,
  // the block size is doubled every time
  // if allocation fails, block size is reduced by 1/4
  void allocate() {
    try {
      if(dataSize.empty()) dataSize.push_back(N);
      else dataSize.push_back(dataSize.back()*2);
      void *m=malloc(sizeof(Object)*dataSize.back());
      while(!m) {
        dataSize.back()=static_cast<size_t>(dataSize.back()*0.75);
        m=malloc(sizeof(Object)*dataSize.back());
      }
      data.push_back(static_cast<Object*>(m));
    } catch (const std::exception& e) {
      TRACE_ERR("caught std::exception: "<<e.what()
                <<" in ObjectPool::allocate(), name: "<<name<<", last size: "
                <<dataSize.back()<<"\n");
      TRACE_ERR("OPOOL info: "<<data.size()<<" "<<dataSize.size()<<" "
                <<freeObj.size()<<"\n"<<idx<<" "<<dIdx<<" "<<N<<"\n");
      std::copy(dataSize.begin(),dataSize.end(),
                std::ostream_iterator<size_t>(std::cerr," "));
      TRACE_ERR("\n");
      throw;
    }
  }
};

#endif
