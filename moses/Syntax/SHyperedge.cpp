#include "SHyperedge.h"

#include "moses/StaticData.h"

#include "SVertex.h"

namespace Moses
{
namespace Syntax
{

Phrase GetOneBestTargetYield(const SHyperedge &h)
{
  FactorType placeholderFactor = StaticData::Instance().options()->input.placeholder_factor;

  Phrase ret(ARRAY_SIZE_INCR);

  const AlignmentInfo::NonTermIndexMap &targetToSourceMap =
    h.label.translation->GetAlignNonTerm().GetNonTermIndexMap2();

  for (std::size_t pos = 0; pos < h.label.translation->GetSize(); ++pos) {
    const Word &word = h.label.translation->GetWord(pos);
    if (word.IsNonTerminal()) {
      std::size_t sourceIndex = targetToSourceMap[pos];
      const SHyperedge &incoming = *h.tail[sourceIndex]->best;
      Phrase subPhrase = GetOneBestTargetYield(incoming);
      ret.Append(subPhrase);
    } else {
      ret.AddWord(word);
      if (placeholderFactor == NOT_FOUND) {
        continue;
      }
      assert(false);
      // FIXME Modify this chunk of code to work for SHyperedge.
      /*
            std::set<std::size_t> sourcePosSet =
              h.translation->GetAlignTerm().GetAlignmentsForTarget(pos);
            if (sourcePosSet.size() == 1) {
              const std::vector<const Word*> *ruleSourceFromInputPath =
                hypo.GetTranslationOption().GetSourceRuleFromInputPath();
              UTIL_THROW_IF2(ruleSourceFromInputPath == NULL,
                             "Source Words in of the rules hasn't been filled out");
              std::size_t sourcePos = *sourcePosSet.begin();
              const Word *sourceWord = ruleSourceFromInputPath->at(sourcePos);
              UTIL_THROW_IF2(sourceWord == NULL,
                             "Null source word at position " << sourcePos);
              const Factor *factor = sourceWord->GetFactor(placeholderFactor);
              if (factor) {
                ret.Back()[0] = factor;
              }
            }
      */
    }
  }
  return ret;
}

}  // Syntax
}  // Moses
