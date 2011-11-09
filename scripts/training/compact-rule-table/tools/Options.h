#pragma once
#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <string>

namespace moses {

struct Options {
 public:
  Options() {}
  std::string inputFile;
  std::string outputFile;
};

}  // namespace moses

#endif
