#include "moses/FeatureVector.h"
#include "moses/ScoreComponentCollection.h"

namespace Mira
{

class Hildreth
{
public :
  static std::vector<float> optimise (const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<float>& b );
  static std::vector<float> optimise (const std::vector<Moses::ScoreComponentCollection>& a, const std::vector<float>& b, float C);
};
}
