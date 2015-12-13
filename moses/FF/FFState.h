#ifndef moses_FFState_h
#define moses_FFState_h

#include <vector>
#include <stddef.h>
#include "util/exception.hh"

namespace Moses
{

class FFState
{
public:
  virtual ~FFState();
  virtual size_t hash() const = 0;
  virtual bool operator==(const FFState& other) const = 0;

  virtual bool operator!=(const FFState& other) const {
    return !(*this == other);
  }
};

class DummyState : public FFState
{
public:
  DummyState()  {}

  virtual size_t hash() const {
    return 0;
  }

  virtual bool operator==(const FFState& other) const {
    return true;
  }

};

}
#endif
