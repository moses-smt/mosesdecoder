
#pragma once

#include <unistd.h>
#include <vector>

class WordsRange;

class WordsBitmap
{
protected:
  std::vector<bool>  m_bitmap;
public:
  WordsBitmap(); // do not implement

  // creating the inital hypo. No words translated
  WordsBitmap(size_t size);
  WordsBitmap(const WordsBitmap &copy);
  WordsBitmap(const WordsBitmap &copy, const WordsRange &range);
  virtual ~WordsBitmap();

  //! count of words translated
  size_t GetNumWordsCovered() const;

  //! count of words translated
  size_t GetFirstGapPos() const;

  bool IsComplete() const {
    return m_bitmap.size() == GetNumWordsCovered();
  }

  bool WithinReorderingConstraint(const WordsRange &prevRange, const WordsRange &nextRange) const;

  //! whether the wordrange overlaps with any translated word in this bitmap
  bool Overlap(const WordsRange &compare) const;

  std::string Debug() const;
};

