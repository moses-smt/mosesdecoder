/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2012 University of Edinburgh
 
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

#include "xml_tree_parser.h"

#include "exception.h"
#include "tables-core.h"
#include "XmlException.h"
#include "XmlTree.h"

#include <cassert>
#include <vector>

using namespace MosesTraining;

namespace Moses {
namespace PCFG {

XmlTreeParser::XmlTreeParser()
{
}

std::auto_ptr<PcfgTree> XmlTreeParser::Parse(const std::string &line)
{
  m_line = line;
  m_tree.Clear();
  try {
    if (!ProcessAndStripXMLTags(m_line, m_tree, m_labelSet, m_topLabelSet)) {
      throw Exception("");
    }
  } catch (const XmlException &e) {
    throw Exception(e.getMsg());
  }
  m_tree.ConnectNodes();
  SyntaxNode *root = m_tree.GetTop();
  if (!root) {
    // There is no XML tree.
    return std::auto_ptr<PcfgTree>();
  }
  m_words = tokenize(m_line.c_str());
  return ConvertTree(*root, m_words);
}

// Converts a SyntaxNode tree to a Moses::PCFG::PcfgTree.
std::auto_ptr<PcfgTree> XmlTreeParser::ConvertTree(
    const SyntaxNode &tree,
    const std::vector<std::string> &words)
{
  std::auto_ptr<PcfgTree> root(new PcfgTree(tree.GetLabel()));
  const std::vector<SyntaxNode*> &children = tree.GetChildren();
  if (children.empty()) {
    if (tree.GetStart() != tree.GetEnd()) {
      std::ostringstream msg;
      msg << "leaf node covers multiple words (" << tree.GetStart()
          << "-" << tree.GetEnd() << "): this is currently unsupported";
      throw Exception(msg.str());
    }
    std::auto_ptr<PcfgTree> leaf(new PcfgTree(words[tree.GetStart()]));
    leaf->set_parent(root.get());
    root->AddChild(leaf.release());
  } else {
    for (std::vector<SyntaxNode*>::const_iterator p = children.begin();
         p != children.end(); ++p) {
      assert(*p);
      std::auto_ptr<PcfgTree> child = ConvertTree(**p, words);
      child->set_parent(root.get());
      root->AddChild(child.release());
    }
  }
  return root;
}

}  // namespace PCFG
}  // namespace Moses
