#include <cstdlib>
#include <ctime>
#include <limits>

#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>

#include "RandomFeature.h"
#include "ScoreComponentCollection.h"
#include "StaticData.h"


using namespace std;

namespace Moses {

  RandomFeature::RandomFeature(size_t numValues, size_t scaling) :
      StatelessFeatureFunction("Random"),
         m_numValues(numValues), m_scaling(scaling) 
  {}

  void RandomFeature::Evaluate(
    const TargetPhrase& target,
    ScoreComponentCollection* accumulator) const
  {
    vector<float> values(m_numValues);
    vector<FactorType> factors(1);
    size_t seed = 0;
    boost::hash_combine(seed,target.GetStringRep(factors));
    srand(seed);
    for (size_t i = 0; i < m_numValues; ++i) {
      values[i] = (float)rand()/RAND_MAX * m_scaling; 
      //cerr << "RAND " << target.GetStringRep(factors) << " " << i << " " << values[i] << endl;
    }
    accumulator->PlusEquals(this,values);
  }

	size_t RandomFeature::GetNumScoreComponents() const {
    return m_numValues;
  }

	string RandomFeature::GetScoreProducerWeightShortName() const {
    return "r";
  }


};
