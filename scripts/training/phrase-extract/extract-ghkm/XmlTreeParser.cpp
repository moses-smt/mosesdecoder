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
#include "XmlTree.h"

#include <cassert>
#include <vector>

namespace Moses {
namespace GHKM {

std::auto_ptr<ParseTree> ParseXmlTree(
    std::vector<std::string>::const_iterator &p,
    const std::vector<std::string>::const_iterator &end)
{
  std::auto_ptr<ParseTree> t;

  if (p == end) {
    return t;
  }

  std::string s(Trim(*p));

  while (s.empty()) {
    s = Trim(*++p);
  }

  if (!isXmlTag(s)) {
    p++;
    t.reset(new ParseTree(unescape(s)));
    return t;
  }

  const std::string &tag = s;

  if (tag[1] == '/') {
    // Closing tag.  Don't advance p -- let caller handle it.
    return t;
  }

  std::string label = ParseXmlTagAttribute(tag, "label");
  t.reset(new ParseTree(label));

  if (tag[tag.size()-2] == '/') {
    // Unary tag.
    p++;
    return t;
  }

  p++;
  while (ParseTree *c = ParseXmlTree(p, end).release()) {
    t->AddChild(c);
    c->SetParent(t.get());
  }
  p++;  // Skip over closing tag

  return t;
}

std::auto_ptr<ParseTree> ParseXmlTree(const std::string &line)
{
  std::vector<std::string> xmlTokens(TokenizeXml(line));
  std::vector<std::string>::const_iterator begin(xmlTokens.begin());
  std::vector<std::string>::const_iterator end(xmlTokens.end());
  return ParseXmlTree(begin, end);
}

}  // namespace GHKM
}  // namespace Moses
