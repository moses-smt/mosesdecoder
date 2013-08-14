// $Id: XmlOption.cpp 1960 2008-12-15 12:52:38Z phkoehn $
// vim:tabstop=2

/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2006 University of Edinburgh

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
#include <string>
#include <vector>
#include <set>
#include <map>
#include "SyntaxTree.h"

namespace MosesTraining
{

std::string ParseXmlTagAttribute(const std::string& tag,const std::string& attributeName);
std::string Trim(const std::string& str, const std::string dropChars = " \t\n\r");
std::string TrimXml(const std::string& str);
bool isXmlTag(const std::string& tag);
std::vector<std::string> TokenizeXml(const std::string& str);
bool ProcessAndStripXMLTags(std::string &line, SyntaxTree &tree, std::set< std::string > &labelCollection, std::map< std::string, int > &topLabelCollection, bool unescape = true);
std::string unescape(const std::string &str);


} // namespace

