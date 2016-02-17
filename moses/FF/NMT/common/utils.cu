#include "utils.h"

void Trim(std::string& s) {
  boost::trim_left_if(s, boost::is_any_of(" \t\n"));
}

void Split(const std::string& line, std::vector<std::string>& pieces, const std::string del) {
  size_t begin = 0;
  size_t pos = 0;
  std::string token;
  while ((pos = line.find(del, begin)) != std::string::npos) {
    token = line.substr(begin, pos);
    pieces.push_back(token);
    begin = pos + del.size();
  }
  token = line.substr(begin, pos);
  pieces.push_back(line);
}
