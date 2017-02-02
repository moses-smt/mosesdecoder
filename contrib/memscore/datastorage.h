// memscore - in-memory phrase scoring for Statistical Machine Translation
// Christian Hardmeier, FBK-irst, Trento, 2010
// $Id$

#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include "memscore.h"

template<class T>
class DataStorage
{
private:
  T *base_;
  size_t pos_;
  size_t chunk_size_;

  DataStorage() : base_(NULL), pos_(0) {
    int ps = getpagesize();
    chunk_size_ = 10 * ps * sizeof(T);
    base_ = new T[chunk_size_];
  }

  DataStorage(const DataStorage &cc) {
    abort();
  }

  ~DataStorage() {}

public:
  static DataStorage &get_instance() {
    static DataStorage<T> instance;
    return instance;
  }

  T *alloc(size_t count) {
    if(count == 0)
      return NULL;

    // The memory leak is intended.
    if(pos_ + count > chunk_size_) {
      base_ = new T[chunk_size_];
      pos_ = 0;
    }

    T *ret = base_ + pos_;
    pos_ += count;
    return ret;
  }
};

#endif
