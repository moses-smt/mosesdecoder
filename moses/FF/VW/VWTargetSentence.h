#pragma once

#include <vector>

#include "moses/AlignmentInfo.h"
#include "moses/Phrase.h"

#include "AlignmentConstraint.h"

namespace Moses
{

/**
 * VW thread-specific data about target sentence.
 */
class VWTargetSentence
{
public:
  VWTargetSentence() : m_sentence(NULL), m_alignment(NULL) {}

  void Clear() {
    if (m_sentence) delete m_sentence;
    if (m_alignment) delete m_alignment;
  }

  ~VWTargetSentence() {
    Clear();
  }

  void SetConstraints(size_t sourceSize) {
    // initialize to unconstrained
    m_sourceConstraints.assign(sourceSize, AlignmentConstraint());
    m_targetConstraints.assign(m_sentence->GetSize(), AlignmentConstraint());

    // set constraints according to alignment points
    AlignmentInfo::const_iterator it;
    for (it = m_alignment->begin(); it != m_alignment->end(); it++) {
      int src = it->first;
      int tgt = it->second;

      if (src >= m_sourceConstraints.size() || tgt >= m_targetConstraints.size()) {
        UTIL_THROW2("VW :: alignment point out of bounds: " << src << "-" << tgt);
      }

      m_sourceConstraints[src].Update(tgt);
      m_targetConstraints[tgt].Update(src);
    }
  }

  Phrase *m_sentence;
  AlignmentInfo *m_alignment;
  std::vector<AlignmentConstraint> m_sourceConstraints, m_targetConstraints;
};

}
