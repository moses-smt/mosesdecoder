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
  m_nodeCollection.Clear();
  try {
    if (!ProcessAndStripXMLTags(m_line, m_nodeCollection, m_labelSet,
                                m_topLabelSet, false)) {
      throw Exception("");
    }
  } catch (const XmlException &e) {
    throw Exception(e.getMsg());
  }
  std::auto_ptr<SyntaxTree> root = m_nodeCollection.ExtractTree();
  m_words = util::tokenize(m_line);
  AttachWords(m_words, *root);
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
