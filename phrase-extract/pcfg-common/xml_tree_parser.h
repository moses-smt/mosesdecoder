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

#pragma once
#ifndef PCFG_XML_TREE_PARSER_H_
#define PCFG_XML_TREE_PARSER_H_

#include "pcfg_tree.h"
#include "SyntaxTree.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Moses
{
namespace PCFG
{

// Parses a string in Moses' XML parse tree format and returns a PcfgTree
// object.
class XmlTreeParser
{
public:
  XmlTreeParser();
  std::auto_ptr<PcfgTree> Parse(const std::string &);
private:
  std::auto_ptr<PcfgTree> ConvertTree(const MosesTraining::SyntaxNode &,
                                      const std::vector<std::string> &);

  std::set<std::string> m_labelSet;
  std::map<std::string, int> m_topLabelSet;
  std::string m_line;
  MosesTraining::SyntaxTree m_tree;
  std::vector<std::string> m_words;
};

}  // namespace PCFG
}  // namespace Moses

#endif
