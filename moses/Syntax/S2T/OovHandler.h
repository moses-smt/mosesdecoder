#pragma once

#include <string>

#include <boost/shared_ptr.hpp>

#include "moses/Phrase.h"
#include "moses/Syntax/RuleTableFF.h"
#include "moses/TargetPhrase.h"
#include "moses/Word.h"

#include "RuleTrieCreator.h"

namespace Moses
{
namespace Syntax
{
namespace S2T
{

template<typename RuleTrie>
class OovHandler : public RuleTrieCreator
{
public:
  OovHandler(const RuleTableFF &ff) : m_ruleTableFF(ff) {}

  // Synthesize a RuleTrie given a sequence of OOV words.  The sequence is
  // specified by a pair of iterators (indicating the beginning and end).  It
  // is assumed not to contain duplicates.
  template<typename InputIterator>
  boost::shared_ptr<RuleTrie> SynthesizeRuleTrie(InputIterator, InputIterator);

private:
  const RuleTableFF &m_ruleTableFF;

  bool ShouldDrop(const Word &);

  Phrase *SynthesizeSourcePhrase(const Word &);

  Word *SynthesizeTargetLhs(const std::string &);

  TargetPhrase *SynthesizeTargetPhrase(const Word &, const Phrase &,
                                       const Word &, float);
};

}  // S2T
}  // Syntax
}  // Moses

#include "OovHandler-inl.h"
