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

// Parses a string in Moses' XML parse tree format and returns a SyntaxTree
// object.  This is a wrapper around the ProcessAndStripXMLTags function.
class XmlTreeParser {
 public:
  XmlTreeParser(std::set<std::string> &, std::map<std::string, int> &);

  std::auto_ptr<SyntaxTree> Parse(const std::string &);

  const std::vector<std::string>& GetWords() {
    return words_;
  }

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
