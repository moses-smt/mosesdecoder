#pragma once

#include <vector>
#include "Word.h"

// a vector of terminals
class Phrase : public std::vector<Word*>
{
public:
  Phrase() {
  }

  Phrase(size_t size)
    :std::vector<Word*>(size) {
  }

  std::string Debug() const;

};
