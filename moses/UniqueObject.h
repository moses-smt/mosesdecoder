/* ---------------------------------------------------------------- */
/* Copyright 2004 (c) by RWTH Aachen - Lehrstuhl fuer Informatik VI */
/* Richard Zens                                                     */
/* ---------------------------------------------------------------- */
#ifndef moses_UniqueObject_h
#define moses_UniqueObject_h

#include <iostream>
#include <set>

template<class T>  T const* uniqueObject(const T& x,int mode=0)
{
  typedef std::set<T> Pool;

  static Pool pool;
  static size_t Size=0;

  if(mode==0) {
    std::pair<typename Pool::iterator,bool> p=pool.insert(x);
    if(p.second && (++Size%100000==0))
      std::cerr<<"uniqueObjects -- size: "<<Size<<" object size: "<<sizeof(T)<<"\n";

    return &(*(p.first));
  } else {
    pool.clear();
    Size=0;
    return 0;
  }
}

//! @todo what is this?
template<class T> class UniqueObjectManager
{
public:
  typedef T Object;
private:
  typedef std::set<T> Pool;
  Pool pool;
public:
  UniqueObjectManager() {}

  void clear() {
    pool.clear();
  }
  size_t size() const {
    return pool.size();
  }

  Object const * operator()(const Object& x) {
#ifdef DEBUG
    std::pair<typename Pool::iterator,bool> p=pool.insert(x);
    if(p.second && (size()%100000==0))
      std::cerr<<"uniqueObjects -- size: "<<size()<<" object size: "<<sizeof(Object)<<"\n";
    return &(*(p.first));
#else
    return  &(*(pool.insert(x).first));
#endif
  }
};



#endif
