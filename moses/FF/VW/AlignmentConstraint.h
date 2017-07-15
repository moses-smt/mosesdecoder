#pragma once

namespace Moses
{

/**
 * Helper class for storing alignment constraints.
 */
class AlignmentConstraint
{
public:
  AlignmentConstraint() : m_min(std::numeric_limits<int>::max()), m_max(-1) {}

  AlignmentConstraint(int min, int max) : m_min(min), m_max(max) {}

  /**
   * We are aligned to point => our min cannot be larger, our max cannot be smaller.
   */
  void Update(int point) {
    if (m_min > point) m_min = point;
    if (m_max < point) m_max = point;
  }

  bool IsSet() const {
    return m_max != -1;
  }

  int GetMin() const {
    return m_min;
  }

  int GetMax() const {
    return m_max;
  }

private:
  int m_min, m_max;
};

}
