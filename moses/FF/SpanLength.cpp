#include <boost/shared_ptr.hpp>
#include "SpanLength.h"
#include "moses/StaticData.h"
#include "moses/Word.h"
#include "moses/ChartCellLabel.h"
#include "moses/WordsRange.h"
#include "moses/StackVec.h"
#include "moses/TargetPhrase.h"
#include "moses/PP/PhraseProperty.h"

using namespace std;

namespace Moses
{
SpanLength::SpanLength(const std::string &line)
:StatelessFeatureFunction(1, line)
,m_smoothingMethod(None)
{
}

void SpanLength::Evaluate(const Phrase &source
						, const TargetPhrase &targetPhrase
						, ScoreComponentCollection &scoreBreakdown
						, ScoreComponentCollection &estimatedFutureScore) const
{
  targetPhrase.SetRuleSource(source);
}

void SpanLength::Evaluate(const InputType &input
                                   , const InputPath &inputPath
                                   , const TargetPhrase &targetPhrase
                                   , const StackVec *stackVec
                                   , ScoreComponentCollection &scoreBreakdown
                                   , ScoreComponentCollection *estimatedFutureScore) const
{
  assert(stackVec);

  boost::shared_ptr<PhraseProperty> property;
  bool hasProperty = targetPhrase.GetProperty("SpanLength", property);

  if (!hasProperty) {
	  return;
  }

  string str = property.get()->GetValueString();
  cerr << "str=" << str << endl;

  const Phrase *ruleSource = targetPhrase.GetRuleSource();
  assert(ruleSource);
  cerr << *ruleSource << endl;

  for (size_t i = 0; i < stackVec->size(); ++i) {
	  const ChartCellLabel &cell = *stackVec->at(i);
	  const WordsRange &ntRange = cell.GetCoverage();


  }

}

void SpanLength::SetParameter(const std::string& key, const std::string& value)
{
  if (key == "smoothing") {
	  if (value == "plus-constant") {
		  m_smoothingMethod = PlusConst;
	  }
	  else if (value == "none") {
		  m_smoothingMethod = None;
	  }
	  else {
		  UTIL_THROW(util::Exception, "Unknown smoothing type " << value);
	  }
  }
  else if (key == "constant") {
	  m_const = Scan<float>(value);
  }
  else {
    StatelessFeatureFunction::SetParameter(key, value);
  }
}

}

