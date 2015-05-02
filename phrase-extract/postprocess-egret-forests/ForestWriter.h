#pragma once

#include <ostream>
#include <string>

#include "Forest.h"
#include "Options.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

class ForestWriter
{
public:
  ForestWriter(const Options &options, std::ostream &out)
    : m_options(options), m_out(out) {}

  void Write(const std::string &, const Forest &, std::size_t);

private:
  std::string Escape(const std::string &) const;
  std::string PossiblyEscape(const std::string &) const;
  void WriteHyperedgeLine(const Forest::Hyperedge &);
  void WriteVertex(const Forest::Vertex &);

  const Options &m_options;
  std::ostream &m_out;
};

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
