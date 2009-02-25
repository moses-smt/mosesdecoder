#include "InputSource.h"

#include <string>
#include <istream>

namespace Josiah {

InputSource::~InputSource() {}

StreamInputSource::StreamInputSource(std::istream& is) : in(is) {
}

bool StreamInputSource::HasMore() const {
  return (in);
}

void StreamInputSource::GetSentence(std::string* sentence, int* lineno) {
  (void) lineno;
  std::getline(in, *sentence);
};

}

