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
  
  /// should return -1, 0, 1 for (<, ==, >) respectively
  virtual int Compare(const FFState& other) const = 0;
};

}
#endif
