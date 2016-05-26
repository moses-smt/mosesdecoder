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

#include "LoaderFactory.h"

#include "moses/Util.h"
#include "moses/InputFileStream.h"
#include "LoaderCompact.h"
#include "LoaderHiero.h"
#include "LoaderStandard.h"

#include <sstream>
#include <iostream>

using namespace std;

namespace Moses
{

// Determines the rule table type by peeking inside the file then creates
// a suitable RuleTableLoader object.
std::auto_ptr<RuleTableLoader>
RuleTableLoaderFactory::
Create(const std::string &path)
{
  InputFileStream input(path);
  std::string line;

  if (std::getline(input, line)) {
    std::vector<std::string> tokens;
    Tokenize(tokens, line);
    if (tokens.size() == 1) {
      if (tokens[0] == "1") {
        return std::auto_ptr<RuleTableLoader>(new RuleTableLoaderCompact());
      }
      std::cerr << "Unsupported compact rule table format: " << tokens[0];
      return std::auto_ptr<RuleTableLoader>();
    } else if (tokens[0] == "[X]" && tokens[1] == "|||") {
      return std::auto_ptr<RuleTableLoader>(new RuleTableLoaderHiero());
    }

    return std::auto_ptr<RuleTableLoader>(new RuleTableLoaderStandard());
  } else {
    // empty phrase table
    return std::auto_ptr<RuleTableLoader>(new RuleTableLoaderStandard());
  }
}

}  // namespace Moses
