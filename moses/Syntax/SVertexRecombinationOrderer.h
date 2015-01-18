#pragma once

#include "moses/FF/FFState.h"

#include "SVertex.h"

namespace Moses
{
namespace Syntax
{

struct SVertexRecombinationOrderer {
public:
  bool operator()(const SVertex &x, const SVertex &y) const {
    int comp = 0;
    for (std::size_t i = 0; i < x.state.size(); ++i) {
      if (x.state[i] == NULL || y.state[i] == NULL) {
        comp = x.state[i] - y.state[i];
      } else {
        comp = x.state[i]->Compare(*y.state[i]);
      }
      if (comp != 0) {
        return comp < 0;
      }
    }
    return false;
  }

  bool operator()(const SVertex *x, const SVertex *y) const {
    return operator()(*x, *y);
  }
};

}  // Syntax
}  // Moses
