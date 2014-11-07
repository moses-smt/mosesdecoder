#pragma once

#include "SHyperedgeBundle.h"

namespace Moses
{
namespace Syntax
{

struct SHyperedgeBundleScorer
{
 public:
  static float Score(const SHyperedgeBundle &bundle) {
    const TargetPhrase &targetPhrase = **(bundle.translations->begin());
    float score = targetPhrase.GetFutureScore();
    for (std::vector<const SVertexBeam*>::const_iterator p =
          bundle.beams.begin(); p != bundle.beams.end(); ++p) {
      const SVertexBeam *beam = *p;
      if (beam->front()->best) {
        score += beam->front()->best->score;
      }
    }
    return score;
  }
};

}  // Syntax
}  // Moses
