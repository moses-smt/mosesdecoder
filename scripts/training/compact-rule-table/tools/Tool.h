#pragma once
#ifndef TOOL_H_
#define TOOL_H_

#include <cstdlib>
#include <iostream>
#include <string>

namespace moses {

class Tool {
 public:
  Tool(const std::string &name) : m_name(name) {}

  const std::string &getName() const { return m_name; }

  virtual int main(int argc, char *argv[]) = 0;

  void warn(const std::string &msg) const {
    std::cerr << m_name << ": warning: " << msg << std::endl;
  }

  void error(const std::string &msg) const {
    std::cerr << m_name << ": error: " << msg << std::endl;
    std::exit(1);
  }

 private:
  std::string m_name;
};

}  // namespace moses

#endif
