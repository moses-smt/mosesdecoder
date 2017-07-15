#include <sstream>
#include "Phrase.h"

std::string Phrase::Debug() const
{
  std::stringstream out;

  for (size_t i = 0; i < size(); ++i) {
    Word &word = *at(i);
    out << word.Debug() << " ";
  }

  return out.str();
}
