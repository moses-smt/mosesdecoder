#pragma once

#include <vector>
#include <stddef.h>
#include "util/exception.hh"

namespace Moses2
{

class FFState
{
public:
  virtual ~FFState() {
  }
  virtual size_t hash() const = 0;
  virtual bool operator==(const FFState& other) const = 0;

  virtual bool operator!=(const FFState& other) const {
    return !(*this == other);
  }

  virtual std::string ToString() const = 0;
};

////////////////////////////////////////////////////////////////////////////////////////
inline std::ostream& operator<<(std::ostream& out, const FFState& obj)
{
  out << obj.ToString();
  return out;
}

////////////////////////////////////////////////////////////////////////////////////////
class DummyState: public FFState
{
public:
  DummyState() {
  }

  virtual size_t hash() const {
    return 0;
  }

  virtual bool operator==(const FFState& other) const {
    return true;
  }

};

}

