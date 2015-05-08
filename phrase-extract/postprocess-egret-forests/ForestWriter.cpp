#include "ForestWriter.h"

#include <cassert>
#include <vector>

#include "TopologicalSorter.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

void ForestWriter::Write(const std::string &sentence, const Forest &forest,
                         std::size_t sentNum)
{
  m_out << "sentence " << sentNum << " :" << std::endl;
  m_out << PossiblyEscape(sentence) << std::endl;

  // Check for parse failure.
  if (forest.vertices.empty()) {
    m_out << std::endl << std::endl;
    return;
  }

  // Sort the vertices topologically then output the hyperedges from each.
  std::vector<const Forest::Vertex *> vertices;
  TopologicalSorter sorter;
  sorter.Sort(forest, vertices);
  for (std::vector<const Forest::Vertex *>::const_iterator p = vertices.begin();
       p != vertices.end(); ++p) {
    const Forest::Vertex &v = **p;
    for (std::vector<boost::shared_ptr<Forest::Hyperedge> >::const_iterator
         q = v.incoming.begin(); q != v.incoming.end(); ++q) {
      WriteHyperedgeLine(**q);
    }
  }

  // Write a terminating blank line.
  m_out << std::endl;
}

void ForestWriter::WriteHyperedgeLine(const Forest::Hyperedge &e)
{
  WriteVertex(*e.head);
  m_out << " =>";
  for (std::vector<Forest::Vertex *>::const_iterator p = e.tail.begin();
       p != e.tail.end(); ++p) {
    m_out << " ";
    WriteVertex(**p);
  }
  m_out << " ||| " << e.weight << std::endl;
}

void ForestWriter::WriteVertex(const Forest::Vertex &v)
{
  m_out << PossiblyEscape(v.symbol.value);
  if (!v.incoming.empty()) {
    m_out << "[" << v.start << "," << v.end << "]";
  }
}

std::string ForestWriter::PossiblyEscape(const std::string &s) const
{
  if (m_options.escape) {
    return Escape(s);
  } else {
    return s;
  }
}

// Escapes XML special characters.
std::string ForestWriter::Escape(const std::string &s) const
{
  std::string t;
  std::size_t len = s.size();
  t.reserve(len);
  for (std::size_t i = 0; i < len; ++i) {
    if (s[i] == '<') {
      t += "&lt;";
    } else if (s[i] == '>') {
      t += "&gt;";
    } else if (s[i] == '[') {
      t += "&#91;";
    } else if (s[i] == ']') {
      t += "&#93;";
    } else if (s[i] == '|') {
      t += "&#124;";
    } else if (s[i] == '&') {
      t += "&amp;";
    } else if (s[i] == '\'') {
      t += "&apos;";
    } else if (s[i] == '"') {
      t += "&quot;";
    } else {
      t += s[i];
    }
  }
  return t;
}

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
