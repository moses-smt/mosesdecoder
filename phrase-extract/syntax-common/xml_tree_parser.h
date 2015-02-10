#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "SyntaxTree.h"

#include "exception.h"
#include "string_tree.h"

namespace MosesTraining {
namespace Syntax {

// Parses a string in Moses' XML parse tree format and returns a StringTree
// object.  This is a wrapper around the ProcessAndStripXMLTags function.
class XmlTreeParser {
 public:
  StringTree *Parse(const std::string &);

 private:
  static StringTree *ConvertTree(const MosesTraining::SyntaxNode &,
                                 const std::vector<std::string> &);

  std::set<std::string> label_set_;
  std::map<std::string, int> top_label_set_;
  std::string line_;
  MosesTraining::SyntaxTree tree_;
  std::vector<std::string> words_;
};

}  // namespace Syntax
}  // namespace MosesTraining
