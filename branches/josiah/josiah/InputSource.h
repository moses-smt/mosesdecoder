#pragma once

#include <string>
#include <istream>

namespace Josiah {

struct InputSource {
  virtual bool HasMore() const = 0;
  virtual void GetSentence(std::string* sentence, int* lineno) = 0;
  virtual ~InputSource();
};

struct StreamInputSource : public InputSource {
  std::istream& in;
  StreamInputSource(std::istream& is);
  virtual bool HasMore() const;
  virtual void GetSentence(std::string* sentence, int* lineno);
};

}

