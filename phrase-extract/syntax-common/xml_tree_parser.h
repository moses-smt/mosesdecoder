#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "SyntaxNode.h"
#include "SyntaxNodeCollection.h"
#include "SyntaxTree.h"

#include "exception.h"

namespace MosesTraining {
namespace Syntax {

/** Parses string representations of parse trees in Moses' XML format and
 *  converts them to SyntaxTree objects.
 *
 *  This is a thin wrapper around the ProcessAndStripXMLTags function.  After
 *  calling Parse(), the output of the ProcessAndStripXMLTags function (the
 *  sentence, node collection, label set, and top label set) are available via
 *  accessors.
 */
class XmlTreeParser {
 public:
  XmlTreeParser(std::set<std::string> &, std::map<std::string, int> &);

  //! Parse a single sentence and return a SyntaxTree (with words attached).
  std::auto_ptr<SyntaxTree> Parse(const std::string &, bool=false);

  // TODO
  //! Get the sentence string (see ProcessAndStripXMLTags)
  //const std::string &sentence() const;

  // FIXME
  //! Get the sentence as a vector of tokens
  const std::vector<std::string>& GetWords() { return words_; }

  // TODO
  //! Get the node collection (see ProcessAndStripXMLTags)
  const SyntaxNodeCollection &node_collection() const;

  // TODO
  //! Get the label set (see ProcessAndStripXMLTags)
  const std::set<std::string> &label_set() const;

  // TODO
  //! Get the top label set (see ProcessAndStripXMLTags)
  const std::map<std::string, int> &top_label_set() const;

  // FIXME
  const SyntaxNodeCollection &GetNodeCollection() const {
    return node_collection_;
  }

 private:
  std::set<std::string> &label_set_;
  std::map<std::string, int> &top_label_set_;
  std::string line_;
  SyntaxNodeCollection node_collection_;
  std::vector<std::string> words_;

  void AttachWords(const std::vector<std::string> &, SyntaxTree &);
};

}  // namespace Syntax
}  // namespace MosesTraining
