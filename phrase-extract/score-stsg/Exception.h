#pragma once

#include <string>

namespace Moses
{
namespace ScoreStsg
{

class Exception
{
public:
  Exception(const char *msg) : m_msg(msg) {}
  Exception(const std::string &msg) : m_msg(msg) {}
  const std::string &GetMsg() const {
    return m_msg;
  }
private:
  std::string m_msg;
};

} // namespace ScoreStsg
} // namespace Moses
