#include <vector>

#include "StaticData.h"
#include "BaseManager.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/FF/StatefulFeatureFunction.h"

using namespace std;

namespace Moses
{
void BaseManager::OutputAllFeatureScores(const Moses::ScoreComponentCollection &features,
                            std::ostream &out) const
{
  std::string lastName = "";
  const vector<const StatefulFeatureFunction*>& sff = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for( size_t i=0; i<sff.size(); i++ ) {
    const StatefulFeatureFunction *ff = sff[i];
    if (ff->GetScoreProducerDescription() != "BleuScoreFeature"
        && ff->IsTuneable()) {
      OutputFeatureScores( out, features, ff, lastName );
    }
  }
  const vector<const StatelessFeatureFunction*>& slf = StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for( size_t i=0; i<slf.size(); i++ ) {
    const StatelessFeatureFunction *ff = slf[i];
    if (ff->IsTuneable()) {
      OutputFeatureScores( out, features, ff, lastName );
    }
  }
}

void BaseManager::OutputFeatureScores( std::ostream& out,
                          const ScoreComponentCollection &features,
                          const FeatureFunction *ff,
                          std::string &lastName ) const
{
  const StaticData &staticData = StaticData::Instance();
  bool labeledOutput = staticData.IsLabeledNBestList();

  // regular features (not sparse)
  if (ff->GetNumScoreComponents() != 0) {
    if( labeledOutput && lastName != ff->GetScoreProducerDescription() ) {
      lastName = ff->GetScoreProducerDescription();
      out << " " << lastName << "=";
    }
    vector<float> scores = features.GetScoresForProducer( ff );
    for (size_t j = 0; j<scores.size(); ++j) {
      out << " " << scores[j];
    }
  }

  // sparse features
  const FVector scores = features.GetVectorForProducer( ff );
  for(FVector::FNVmap::const_iterator i = scores.cbegin(); i != scores.cend(); i++) {
    out << " " << i->first << "= " << i->second;
  }
}

/***
 * print surface factor only for the given phrase
 */
void BaseManager::OutputSurface(std::ostream &out, const Phrase &phrase,
							const std::vector<FactorType> &outputFactorOrder,
							bool reportAllFactors) const
{
  UTIL_THROW_IF2(outputFactorOrder.size() == 0,
		  "Cannot be empty phrase");
  if (reportAllFactors == true) {
    out << phrase;
  } else {
    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);
      out << *factor;
      UTIL_THROW_IF2(factor == NULL,
    		  "Empty factor 0 at position " << pos);

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
        UTIL_THROW_IF2(factor == NULL,
      		  "Empty factor " << i << " at position " << pos);

        out << "|" << *factor;
      }
      out << " ";
    }
  }
}

// Emulates the old operator<<(ostream &, const DottedRule &) function.  The
// output format is a bit odd (reverse order and double spacing between symbols)
// but there are scripts and tools that expect the output of -T to look like
// that.
void BaseManager::WriteApplicationContext(std::ostream &out,
                                        const ApplicationContext &context) const
{
  assert(!context.empty());
  ApplicationContext::const_reverse_iterator p = context.rbegin();
  while (true) {
    out << p->second << "=" << p->first << " ";
    if (++p == context.rend()) {
      break;
    }
    out << " ";
  }
}

} // namespace


