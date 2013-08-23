#pragma once

namespace Moses
{
class Hypothesis;
class TranslationOption;
class InputType;
class TargetPhrase;
class WordsBitmap;

/**
  * Contains all that a feature function can access without affecting recombination.
  * For stateless features, this is all that it can access. Currently this is not
  * used for stateful features, as it would need to be retro-fitted to the LM feature.
  * TODO: Expose source segmentation,lattice path.
  * XXX Don't add anything to the context that would break recombination XXX
 **/
class PhraseBasedFeatureContext
{
  // The context either has a hypothesis (during search), or a TranslationOption and
  // source sentence (during pre-calculation).
  const Hypothesis* m_hypothesis;
  const TranslationOption& m_translationOption;
  const InputType& m_source;

public:
  PhraseBasedFeatureContext(const Hypothesis* hypothesis);

  const Hypothesis& GetHypothesis() const {
    return *m_hypothesis;
  }

};

} // namespace


