#include "FeatureVector.h"
#include "ScoreComponentCollection.h"

namespace Mira {

  class Hildreth {
    public :
      static std::vector<Moses::FValue> optimise (const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<Moses::FValue>& b );
      static std::vector<Moses::FValue> optimise (const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<Moses::FValue>& b, Moses::FValue C);
  };
}
