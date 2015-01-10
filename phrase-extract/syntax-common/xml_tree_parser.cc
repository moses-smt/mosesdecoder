#include "xml_tree_parser.h"

#include "tables-core.h"
#include "XmlException.h"
#include "XmlTree.h"

#include <cassert>
#include <vector>

namespace MosesTraining {
namespace Syntax {

StringTree *XmlTreeParser::Parse(const std::string &line) {
  line_ = line;
  tree_.Clear();
  try {
    if (!ProcessAndStripXMLTags(line_, tree_, label_set_, top_label_set_,
                                false)) {
      throw Exception("");
    }
  } catch (const XmlException &e) {
    throw Exception(e.getMsg());
  }
  tree_.ConnectNodes();
  SyntaxNode *root = tree_.GetTop();
  assert(root);
  words_ = tokenize(line_.c_str());
  return ConvertTree(*root, words_);
}

// Converts a SyntaxNode tree to a StringTree.
StringTree *XmlTreeParser::ConvertTree(const SyntaxNode &tree,
                                       const std::vector<std::string> &words) {
  StringTree *root = new StringTree(tree.GetLabel());
  const std::vector<SyntaxNode*> &children = tree.GetChildren();
  if (children.empty()) {
    if (tree.GetStart() != tree.GetEnd()) {
      std::ostringstream msg;
      msg << "leaf node covers multiple words (" << tree.GetStart()
          << "-" << tree.GetEnd() << "): this is currently unsupported";
      throw Exception(msg.str());
    }
    StringTree *leaf = new StringTree(words[tree.GetStart()]);
    leaf->parent() = root;
    root->children().push_back(leaf);
  } else {
    for (std::vector<SyntaxNode*>::const_iterator p = children.begin();
         p != children.end(); ++p) {
      assert(*p);
      StringTree *child = ConvertTree(**p, words);
      child->parent() = root;
      root->children().push_back(child);
    }
  }
  return root;
}

}  // namespace Syntax
}  // namespace MosesTraining
