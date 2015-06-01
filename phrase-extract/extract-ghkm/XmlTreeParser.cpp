/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include "XmlTreeParser.h"

#include <cassert>
#include <vector>

#include "util/tokenize.hh"

#include "SyntaxTree.h"
#include "tables-core.h"
#include "XmlException.h"
#include "XmlTree.h"

namespace MosesTraining
{
namespace GHKM
{

XmlTreeParser::XmlTreeParser(std::set<std::string> &labelSet,
                             std::map<std::string, int> &topLabelSet)
  : m_labelSet(labelSet)
  , m_topLabelSet(topLabelSet)
{
}

std::auto_ptr<SyntaxTree> XmlTreeParser::Parse(const std::string &line)
{
  m_line = line;
  m_tree.Clear();
  try {
    if (!ProcessAndStripXMLTags(m_line, m_tree, m_labelSet, m_topLabelSet,
                                false)) {
      throw Exception("");
    }
  } catch (const XmlException &e) {
    throw Exception(e.getMsg());
  }
  //boost::shared_ptr<SyntaxTree> root = m_tree.ExtractTree();
  std::auto_ptr<SyntaxTree> root = m_tree.ExtractTree();
  m_words = util::tokenize(m_line);
  AttachWords(m_words, *root);
  return root;
}

// Converts a SyntaxNode tree to a MosesTraining::GHKM::SyntaxTree.
std::auto_ptr<SyntaxTree> XmlTreeParser::ConvertTree(
  const SyntaxNode &tree,
  const std::vector<std::string> &words)
{
  std::auto_ptr<SyntaxTree> root(new SyntaxTree(tree));
  const std::vector<SyntaxNode*> &children = tree.GetChildren();
  if (children.empty()) {
    if (tree.GetStart() != tree.GetEnd()) {
      std::ostringstream msg;
      msg << "leaf node covers multiple words (" << tree.GetStart()
          << "-" << tree.GetEnd() << "): this is currently unsupported";
      throw Exception(msg.str());
    }
    SyntaxNode value(tree.GetStart(), tree.GetStart(), words[tree.GetStart()]);
    std::auto_ptr<SyntaxTree> leaf(new SyntaxTree(value));
    leaf->parent() = root.get();
    root->children().push_back(leaf.release());
  } else {
    for (std::vector<SyntaxNode*>::const_iterator p = children.begin();
         p != children.end(); ++p) {
      assert(*p);
      std::auto_ptr<SyntaxTree> child = ConvertTree(**p, words);
      child->parent() = root.get();
      root->children().push_back(child.release());
    }
  }
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

}  // namespace GHKM
}  // namespace MosesTraining
