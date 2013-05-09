#include "FeatureVector.h"
#include "ScoreComponentCollection.h"
#include "SparseVec.h"

namespace Optimizer {

  class Hildreth {
    public :
      static std::vector<float> optimise (const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<float>& b );
      static std::vector<float> optimise (const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<float>& b, float C);
      static std::vector<float> optimise (const std::vector<Moses::SparseVec>& a, const std::vector<float>& b, float C);
      static std::vector<float> optimise (const std::vector<Moses::SparseVec>& a, const std::vector<float>& b);

      static float inner_product(const Moses::SparseVec& a, const Moses::SparseVec& b);

  };
}
