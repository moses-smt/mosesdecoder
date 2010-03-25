#ifndef moses_FFState_h
#define moses_FFState_h

#include <cassert>
#include <vector>


namespace Moses {

class FFState {
 public:
  virtual ~FFState();
  virtual int Compare(const FFState& other) const = 0;
};

}
#endif
