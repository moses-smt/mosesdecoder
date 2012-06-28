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

}
#endif
