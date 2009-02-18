#include "InputSource.h"

#include <string>
#include <istream>

namespace Josiah {

InputSource::~InputSource() {}

StreamInputSource::StreamInputSource(std::istream& is) : in(is) {
  std::getline(in, next);
}

bool StreamInputSource::HasMore() const {
  return !in.eof();
}

void StreamInputSource::GetSentence(std::string* sentence, int* lineno) {
  (void) lineno;
  sentence->swap(next);
  std::getline(in, next);
};

}

