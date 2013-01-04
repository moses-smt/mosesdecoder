#ifndef moses_FFState_h
#define moses_FFState_h

#include "util/check.hh"
#include <vector>


namespace Moses
{

/** @todo What is the difference between this and the classes in FeatureFunction?
 */
class FFState
{
public:
  virtual ~FFState();
  virtual int Compare(const FFState& other) const = 0;
};

class DummyState : public FFState {
public:
  DummyState()  {}
  int Compare(const FFState& other) const {
  	return 0;
  }
};

}
#endif
