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
#ifndef PCFG_XML_TREE_WRITER_H_
#define PCFG_XML_TREE_WRITER_H_

#include "syntax_tree.h"

#include "XmlTree.h"

#include <cassert>
#include <map>
#include <memory>
#include <ostream>
#include <vector>
#include <string>

namespace Moses
{
namespace PCFG
{

template<typename InputTree>
class XmlOutputHandler
{
public:
  typedef std::map<std::string, std::string> AttributeMap;

  void GetLabel(const InputTree &, std::string &) const;
  void GetAttributes(const InputTree &, AttributeMap &) const;
};

template<typename InputTree>
class XmlTreeWriter : public XmlOutputHandler<InputTree>
{
public:
  typedef XmlOutputHandler<InputTree> Base;
  void Write(const InputTree &, std::ostream &) const;
private:
  std::string Escape(const std::string &) const;
};

template<typename InputTree>
void XmlTreeWriter<InputTree>::Write(const InputTree &tree,
                                     std::ostream &out) const
{
  assert(!tree.IsLeaf());

  // Opening tag

  std::string label;
  Base::GetLabel(tree, label);
  out << "<tree label=\"" << Escape(label) << "\"";

  typename Base::AttributeMap attribute_map;
  Base::GetAttributes(tree, attribute_map);

  for (typename Base::AttributeMap::const_iterator p = attribute_map.begin();
       p != attribute_map.end(); ++p) {
    out << " " << p->first << "=\"" << p->second << "\"";
  }

  out << ">";

  // Children

  const std::vector<InputTree *> &children = tree.children();
  for (typename std::vector<InputTree *>::const_iterator p = children.begin();
       p != children.end(); ++p) {
    InputTree &child = **p;
    if (child.IsLeaf()) {
      Base::GetLabel(child, label);
      out << " " << Escape(label);
    } else {
      out << " ";
      Write(**p, out);
    }
  }

  // Closing tag
  out << " </tree>";

  if (tree.parent() == 0) {
    out << std::endl;
  }
}

// Escapes XML special characters.
template<typename InputTree>
std::string XmlTreeWriter<InputTree>::Escape(const std::string &s) const
{
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
      t += "&bar;";
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

}  // namespace PCFG
}  // namespace Moses

#endif
