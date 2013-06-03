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

#include "ParseTree.h"
#include "tables-core.h"
#include "XmlException.h"
#include "XmlTree.h"

#include <cassert>
#include <vector>

using namespace MosesTraining;

namespace Moses
{
namespace GHKM
{

XmlTreeParser::XmlTreeParser(std::set<std::string> &labelSet,
                             std::map<std::string, int> &topLabelSet)
  : m_labelSet(labelSet)
  , m_topLabelSet(topLabelSet)
{
}

std::auto_ptr<ParseTree> XmlTreeParser::Parse(const std::string &line)
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
  m_tree.ConnectNodes();
  SyntaxNode *root = m_tree.GetTop();
  assert(root);
  m_words = tokenize(m_line.c_str());
  return ConvertTree(*root, m_words);
}

// Converts a SyntaxNode tree to a Moses::GHKM::ParseTree.
std::auto_ptr<ParseTree> XmlTreeParser::ConvertTree(
  const SyntaxNode &tree,
  const std::vector<std::string> &words)
{
  std::auto_ptr<ParseTree> root(new ParseTree(tree.GetLabel()));
  root->SetPcfgScore(tree.GetPcfgScore());
  const std::vector<SyntaxNode*> &children = tree.GetChildren();
  if (children.empty()) {
    if (tree.GetStart() != tree.GetEnd()) {
      std::ostringstream msg;
      msg << "leaf node covers multiple words (" << tree.GetStart()
          << "-" << tree.GetEnd() << "): this is currently unsupported";
      throw Exception(msg.str());
    }
    std::auto_ptr<ParseTree> leaf(new ParseTree(words[tree.GetStart()]));
    leaf->SetParent(root.get());
    root->AddChild(leaf.release());
  } else {
    for (std::vector<SyntaxNode*>::const_iterator p = children.begin();
         p != children.end(); ++p) {
      assert(*p);
      std::auto_ptr<ParseTree> child = ConvertTree(**p, words);
      child->SetParent(root.get());
      root->AddChild(child.release());
    }
  }
  return root;
}

}  // namespace GHKM
}  // namespace Moses
