#pragma once

namespace Moses
{
class ChartHypothesis;
class InputType;
class TargetPhrase;

/**
 * Same as PhraseBasedFeatureContext, but for chart-based Moses.
 **/
class ChartBasedFeatureContext
{
  //The context either has a hypothesis (during search) or a 
  //TargetPhrase and source sentence (during pre-calculation)
  //TODO: should the context also include some info on where the TargetPhrase
  //is anchored (assuming it's lexicalised), which is available at pre-calc?
  const ChartHypothesis* m_hypothesis;
  const TargetPhrase& m_targetPhrase;
  const InputType& m_source;

public:
  ChartBasedFeatureContext(const ChartHypothesis* hypothesis);
  ChartBasedFeatureContext(const TargetPhrase& targetPhrase,
                           const InputType& source);

  const InputType& GetSource() const;
  const TargetPhrase& GetTargetPhrase() const;

};

} // namespace


