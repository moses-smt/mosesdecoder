#pragma once

#include "SHyperedgeBundle.h"

namespace Moses
{
namespace Syntax
{

struct SHyperedgeBundleScorer {
public:
  static float Score(const SHyperedgeBundle &bundle) {
    const TargetPhrase &targetPhrase = **(bundle.translations->begin());
    float score = targetPhrase.GetFutureScore();
    for (std::vector<const SVertexStack*>::const_iterator p =
           bundle.stacks.begin(); p != bundle.stacks.end(); ++p) {
      const SVertexStack *stack = *p;
      if (stack->front()->best) {
        score += stack->front()->best->score;
      }
    }
    return score;
  }
};

}  // Syntax
}  // Moses
