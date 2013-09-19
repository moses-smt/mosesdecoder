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

#include "RuleTableLoaderFactory.h"

#include "InputFileStream.h"
#include "RuleTableLoaderCompact.h"
#include "RuleTableLoaderHiero.h"

//f+n : include rule table MBOT
#include "RuleTableLoaderMBOT.h"

#include "RuleTableLoaderStandard.h"
#include "UserMessage.h"
#include "Util.h"

#include <sstream>

namespace Moses
{

// Determines the rule table type by peeking inside the file then creates
// a suitable RuleTableLoader object.
std::auto_ptr<RuleTableLoader> RuleTableLoaderFactory::Create(
    const std::string &path)
{

  //std::cout << "Creating rule table loader" << std::endl;

  InputFileStream input(path);
  std::string line;
  std::getline(input, line);
  std::vector<std::string> tokens;
  Tokenize(tokens, line);

  //std::cout << "First token " << tokens[0] << std::endl;
  //std::cout << "Second token " << tokens[1] << std::endl;

  if (tokens.size() == 1) {
    if (tokens[0] == "1") {
      return std::auto_ptr<RuleTableLoader>(new RuleTableLoaderCompact());
    }
    std::stringstream msg;
    msg << "Unsupported compact rule table format: " << tokens[0];
    UserMessage::Add(msg.str());
    return std::auto_ptr<RuleTableLoader>();
  }
  else if (tokens[0] == "[X]" && tokens[1] == "|||") {
    return std::auto_ptr<RuleTableLoader>(new
        RuleTableLoaderHiero());

  }
  else if (tokens[0] == "[MBOT]" && tokens[1] == "|||") {

      //std::cout << "RULE TABLE FACTORY : Loading MBOT table" << std::endl;

    return std::auto_ptr<RuleTableLoader>(new
    RuleTableLoaderMBOT());
  }
  return std::auto_ptr<RuleTableLoader>(new RuleTableLoaderStandard());
}

}  // namespace Moses
