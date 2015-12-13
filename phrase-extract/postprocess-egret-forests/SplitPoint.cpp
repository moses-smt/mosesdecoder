#include "SplitPoint.h"

#include <map>
#include <set>
#include <sstream>

#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "syntax-common/exception.h"

namespace MosesTraining
{
namespace Syntax
{
namespace PostprocessEgretForests
{

void MarkSplitPoints(const std::vector<SplitPoint> &splitPoints,
                     std::string &sentence)
{
  if (splitPoints.empty()) {
    return;
  }

  // FIXME Assumes all split points have same connector
  std::string connector;
  std::map<int, std::set<int> > points;
  for (std::vector<SplitPoint>::const_iterator p = splitPoints.begin();
       p != splitPoints.end(); ++p) {
    points[p->tokenPos].insert(p->charPos);
    connector = p->connector;
  }

  // Split the sentence in to a sequence of tokens.
  std::vector<std::string> terminals;
  const util::AnyCharacter delim(" \t");
  for (util::TokenIter<util::AnyCharacter, true> p(sentence, delim); p; ++p) {
    terminals.resize(terminals.size()+1);
    p->CopyToString(&terminals.back());
  }

  // Mark the split points.
  for (std::map<int, std::set<int> >::const_iterator p = points.begin();
       p != points.end(); ++p) {
    std::string &word = terminals[p->first];
    int offset = 0;
    for (std::set<int>::const_iterator q = p->second.begin();
         q != p->second.end(); ++q) {
      std::string str = std::string("@") + connector + std::string("@");
      word.replace(*q+offset, connector.size(), str);
      offset += 2;
    }
  }

  sentence.clear();
  for (std::size_t i = 0; i < terminals.size(); ++i) {
    if (i > 0) {
      sentence += " ";
    }
    sentence += terminals[i];
  }
}

void MarkSplitPoints(const std::vector<SplitPoint> &splitPoints, Forest &forest)
{
  if (splitPoints.empty()) {
    return;
  }

  // FIXME Assumes all split points have same connector
  std::string connector;
  std::map<int, std::set<int> > points;
  for (std::vector<SplitPoint>::const_iterator p = splitPoints.begin();
       p != splitPoints.end(); ++p) {
    points[p->tokenPos].insert(p->charPos);
    connector = p->connector;
  }

  // Get the terminal vertices in sentence order.
  std::vector<Forest::Vertex *> terminals;
  for (std::vector<boost::shared_ptr<Forest::Vertex> >::const_iterator
       p = forest.vertices.begin(); p != forest.vertices.end(); ++p) {
    if (!(*p)->incoming.empty()) {
      continue;
    }
    int pos = (*p)->start;
    if (pos >= terminals.size()) {
      terminals.resize(pos+1);
    }
    terminals[pos] = p->get();
  }

  // Mark the split points.
  for (std::map<int, std::set<int> >::const_iterator p = points.begin();
       p != points.end(); ++p) {
    std::string &word = terminals[p->first]->symbol.value;
    int offset = 0;
    for (std::set<int>::const_iterator q = p->second.begin();
         q != p->second.end(); ++q) {
      std::string str = std::string("@") + connector + std::string("@");
      word.replace(*q+offset, connector.size(), str);
      offset += 2;
    }
  }

}

}  // namespace PostprocessEgretForests
}  // namespace Syntax
}  // namespace MosesTraining
