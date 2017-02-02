#ifndef moses_Normalizer_h
#define moses_Normalizer_h

#include <vector>
#include <algorithm>
#include "Util.h"

namespace Discriminative
{

class Normalizer
{
public:
  virtual void operator()(std::vector<float> &losses) const = 0;
  virtual ~Normalizer() {}
};

class SquaredLossNormalizer : public Normalizer
{
public:
  virtual void operator()(std::vector<float> &losses) const {
    // This is (?) a good choice for sqrt loss (default loss function in VW)

    float sum = 0;

    // clip to [0,1] and take 1-Z as non-normalized prob
    std::vector<float>::iterator it;
    for (it = losses.begin(); it != losses.end(); it++) {
      if (*it <= 0.0) *it = 1.0;
      else if (*it >= 1.0) *it = 0.0;
      else *it = 1.0 - *it;
      sum += *it;
    }

    if (! Moses::Equals(sum, 0)) {
      // normalize
      for (it = losses.begin(); it != losses.end(); it++)
        *it /= sum;
    } else {
      // sum of non-normalized probs is 0, then take uniform probs
      for (it = losses.begin(); it != losses.end(); it++)
        *it = 1.0 / losses.size();
    }
  }

  virtual ~SquaredLossNormalizer() {}
};

// safe softmax
class LogisticLossNormalizer : public Normalizer
{
public:
  virtual void operator()(std::vector<float> &losses) const {
    std::vector<float>::iterator it;

    float sum = 0;
    float max = 0;
    for (it = losses.begin(); it != losses.end(); it++) {
      *it = -*it;
      max = std::max(max, *it);
    }

    for (it = losses.begin(); it != losses.end(); it++) {
      *it = exp(*it - max);
      sum += *it;
    }

    for (it = losses.begin(); it != losses.end(); it++) {
      *it /= sum;
    }
  }

  virtual ~LogisticLossNormalizer() {}
};

} // namespace Discriminative

#endif // moses_Normalizer_h
