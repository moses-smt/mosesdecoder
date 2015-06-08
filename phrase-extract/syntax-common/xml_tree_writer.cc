#include "xml_tree_writer.h"

#include <cassert>
#include <ostream>
#include <vector>
#include <string>

#include "SyntaxTree.h"
#include "XmlTree.h"


namespace MosesTraining {
namespace Syntax {

void XmlTreeWriter::Write(const SyntaxTree &tree) const {
  assert(!tree.IsLeaf());

  // Opening tag
  out_ << "<tree label=\"" << Escape(tree.value().label) << "\"";
  for (SyntaxNode::AttributeMap::const_iterator
       p = tree.value().attributes.begin();
       p != tree.value().attributes.end(); ++p) {
    if (p->first != "label") {
      out_ << " " << p->first << "=\"" << p->second << "\"";
    }
  }
  out_ << ">";

  // Children
  for (std::vector<SyntaxTree *>::const_iterator p = tree.children().begin();
       p != tree.children().end(); ++p) {
    SyntaxTree &child = **p;
    if (child.IsLeaf()) {
      out_ << " " << Escape(child.value().label);
    } else {
      out_ << " ";
      Write(child);
    }
  }

  // Closing tag
  out_ << " </tree>";

  if (tree.parent() == 0) {
    out_ << std::endl;
  }
}

// Escapes XML special characters.
std::string XmlTreeWriter::Escape(const std::string &s) const {
  if (!escape_) {
    return s;
  }
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

}  // namespace Syntax
}  // namespace MosesTraining
