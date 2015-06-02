#include "xml_tree_parser.h"

#include <cassert>
#include <vector>

#include "util/tokenize.hh"

#include "SyntaxTree.h"
#include "tables-core.h"
#include "XmlException.h"
#include "XmlTree.h"

namespace MosesTraining {
namespace Syntax {

XmlTreeParser::XmlTreeParser(std::set<std::string> &labelSet,
                             std::map<std::string, int> &topLabelSet)
  : label_set_(labelSet)
  , top_label_set_(topLabelSet)
{
}

std::auto_ptr<SyntaxTree> XmlTreeParser::Parse(const std::string &line)
{
  line_ = line;
  node_collection_.Clear();
  try {
    if (!ProcessAndStripXMLTags(line_, node_collection_, label_set_,
                                top_label_set_, false)) {
      throw Exception("");
    }
  } catch (const XmlException &e) {
    throw Exception(e.getMsg());
  }
  std::auto_ptr<SyntaxTree> root = node_collection_.ExtractTree();
  words_ = util::tokenize(line_);
  AttachWords(words_, *root);
  return root;
}

void XmlTreeParser::AttachWords(const std::vector<std::string> &words,
                                SyntaxTree &root)
{
  std::vector<SyntaxTree*> leaves;
  leaves.reserve(words.size());
  for (SyntaxTree::LeafIterator p(root); p != SyntaxTree::LeafIterator(); ++p) {
    leaves.push_back(&*p);
  }

  std::vector<std::string>::const_iterator q = words.begin();
  for (std::vector<SyntaxTree*>::iterator p = leaves.begin(); p != leaves.end();
       ++p) {
    SyntaxTree *leaf = *p;
    const int start = leaf->value().GetStart();
    const int end = leaf->value().GetEnd();
    if (start != end) {
      std::ostringstream msg;
      msg << "leaf node covers multiple words (" << start << "-" << end
          << "): this is currently unsupported";
      throw Exception(msg.str());
    }
    SyntaxTree *newLeaf = new SyntaxTree(SyntaxNode(start, end, *q++));
    leaf->children().push_back(newLeaf);
    newLeaf->parent() = leaf;
  }
}

}  // namespace Syntax
}  // namespace MosesTraining
