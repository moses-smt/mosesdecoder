#pragma once
#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string>

namespace moses {

class Exception {
 public:
  Exception(const char *msg) : m_msg(msg) {}
  Exception(const std::string &msg) : m_msg(msg) {}

  const std::string &getMsg() const { return m_msg; }

 private:
  std::string m_msg;
};

}  // namespace moses

#endif
