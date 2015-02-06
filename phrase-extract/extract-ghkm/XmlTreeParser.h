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

#pragma once
#ifndef EXTRACT_GHKM_XML_TREE_PARSER_H_
#define EXTRACT_GHKM_XML_TREE_PARSER_H_

#include "Exception.h"

#include "SyntaxTree.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Moses
{
namespace GHKM
{

class ParseTree;

// Parses a string in Moses' XML parse tree format and returns a ParseTree
// object.
class XmlTreeParser
{
public:
  XmlTreeParser(std::set<std::string> &, std::map<std::string, int> &);
  std::auto_ptr<ParseTree> Parse(const std::string &);

  static std::auto_ptr<ParseTree> ConvertTree(const MosesTraining::SyntaxNode &,
      const std::vector<std::string> &);

  const std::vector<std::string>& GetWords() {
    return m_words;
  };

private:

  std::set<std::string> &m_labelSet;
  std::map<std::string, int> &m_topLabelSet;
  std::string m_line;
  MosesTraining::SyntaxTree m_tree;
  std::vector<std::string> m_words;
};

}  // namespace GHKM
}  // namespace Moses

#endif
