#include "utils.h"
#include <iostream>

void Trim(std::string& s) {
  boost::trim_left_if(s, boost::is_any_of(" \t\n"));
}

void Split(std::string& line, std::vector<std::string>& pieces, const std::string del) {
  size_t pos = 0;
  std::string token;
  while ((pos = line.find(del)) != std::string::npos) {
      token = line.substr(0, pos);
      pieces.push_back(token);
      line.erase(0, pos + del.size());
  }
  pieces.push_back(line);
}
