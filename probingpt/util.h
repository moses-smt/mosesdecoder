#pragma once
#include <string>
#include <sstream>

namespace probingpt
{

//! convert string to variable of type T. Used to reading floats, int etc from files
template<typename T>
inline T Scan(const std::string &input)
{
  std::stringstream stream(input);
  T ret;
  stream >> ret;
  return ret;
}

//! Specialisation to understand yes/no y/n true/false 0/1
template<>
bool Scan<bool>(const std::string &input);

const std::string ToLower(const std::string& str);

}
