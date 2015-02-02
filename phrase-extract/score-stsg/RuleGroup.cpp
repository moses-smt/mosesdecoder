#include "RuleGroup.h"

namespace MosesTraining
{
namespace Syntax
{
namespace ScoreStsg
{

void RuleGroup::SetNewSource(const StringPiece &source)
{
  source.CopyToString(&m_source);
  m_distinctRules.clear();
  m_totalCount = 0;
}

void RuleGroup::AddRule(const StringPiece &target, const StringPiece &ntAlign,
                        const StringPiece &fullAlign, int count,
                        double treeScore)
{
  if (m_distinctRules.empty() ||
      ntAlign != m_distinctRules.back().ntAlign ||
      target != m_distinctRules.back().target) {
    DistinctRule r;
    target.CopyToString(&r.target);
    ntAlign.CopyToString(&r.ntAlign);
    r.alignments.resize(r.alignments.size()+1);
    fullAlign.CopyToString(&r.alignments.back().first);
    r.alignments.back().second = count;
    r.count = count;
    r.treeScore = treeScore;
    m_distinctRules.push_back(r);
  } else {
    DistinctRule &r = m_distinctRules.back();
    if (r.alignments.back().first != fullAlign) {
      r.alignments.resize(r.alignments.size()+1);
      fullAlign.CopyToString(&r.alignments.back().first);
    }
    r.alignments.back().second += count;
    r.count += count;
  }
  m_totalCount += count;
}

}  // namespace ScoreStsg
}  // namespace Syntax
}  // namespace MosesTraining
