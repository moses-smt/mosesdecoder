#pragma once

#include <istream>
#include <string>
#include <vector>

#include "SplitPoint.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

class SplitPointFileParser
{
public:
  struct Entry {
    std::vector<SplitPoint> splitPoints;
  };

  SplitPointFileParser();
  SplitPointFileParser(std::istream &);

  const Entry &operator*() const {
    return m_entry;
  }
  const Entry *operator->() const {
    return &m_entry;
  }

  SplitPointFileParser &operator++();

  friend bool operator==(const SplitPointFileParser &,
                         const SplitPointFileParser &);

  friend bool operator!=(const SplitPointFileParser &,
                         const SplitPointFileParser &);

private:
  void ParseLine(const std::string &, std::vector<SplitPoint> &);

  Entry m_entry;
  std::istream *m_input;
  std::string m_tmpLine;
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
