
#include <cassert>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <memory.h>
#include "WordsBitmap.h"
#include "WordsRange.h"
#include "TypeDef.h"
#include "Global.h"

using namespace std;

WordsBitmap::WordsBitmap(size_t size)
:m_bitmap(size, false)
{
}

WordsBitmap::WordsBitmap(const WordsBitmap &copy)
:m_bitmap(copy.m_bitmap)
{
}

WordsBitmap::WordsBitmap(const WordsBitmap &copy, const WordsRange &range)
:m_bitmap(copy.m_bitmap)
{
  for (size_t pos = range.startPos; pos <= range.endPos; ++pos) {
    m_bitmap[pos] = true;
  }
}

WordsBitmap::~WordsBitmap()
{
}

size_t WordsBitmap::GetNumWordsCovered() const
{
  size_t count = 0;
  for (size_t pos = 0 ; pos < m_bitmap.size() ; pos++) {
    if (m_bitmap[pos])
      ++count;
  }
  return count;
}

size_t WordsBitmap::GetFirstGapPos() const
{
  for (size_t pos = 0 ; pos < m_bitmap.size() ; pos++) {
    if (!m_bitmap[pos]) {
      return pos;
    }
  }
  // all words translated
  return NOT_FOUND;
}

bool WordsBitmap::WithinReorderingConstraint(const WordsRange &prevRange, const WordsRange &nextRange) const
{
  //return true;
  int maxDistortion = Global::Instance().maxDistortion;
  if (maxDistortion < 0) {
    // unlimited distortion
    return true;
  }

  // actual distortion score if you do create this hypo
  int distScore = prevRange.ComputeDistortionScore(nextRange);
  if (distScore > maxDistortion) {
    return false;
  }

  // what distortion you need to expend to jump back to earler untranslated words
  size_t firstGap = GetFirstGapPos();
  if (nextRange.startPos == firstGap) {
    // no gaps
    return true;
  }

  assert(firstGap < nextRange.endPos);
  if (nextRange.endPos - firstGap + 1 > maxDistortion) {
    // has to jump back too far
    return false;
  }

  return true;
}

bool WordsBitmap::Overlap(const WordsRange &compare) const
{
  for (size_t pos = compare.startPos ; pos <= compare.endPos ; pos++) {
    if (m_bitmap[pos]) {
      return true;
    }
  }
  return false;
}

std::string WordsBitmap::Debug() const
{
  stringstream strme;
  strme << "[";
  for (size_t i = 0; i < m_bitmap.size(); ++i) {
    strme << m_bitmap[i];
  }
  strme << "]";
  return strme.str();

}

