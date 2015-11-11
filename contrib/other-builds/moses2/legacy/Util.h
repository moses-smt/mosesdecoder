#pragma once

#include <limits>

#define NOT_FOUND 			std::numeric_limits<size_t>::max()

template<typename T>
class UnorderedComparer
{
public:
  size_t operator()(const T& obj) const {
    return obj.hash();
  }

  bool operator()(const T& a, const T& b) const {
    return a == b;
  }

  size_t operator()(const T* obj) const {
    return obj->hash();
  }

  bool operator()(const T* a, const T* b) const {
    return (*a) == (*b);
  }

};

