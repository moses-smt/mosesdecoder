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

#include "Alignment.h"

#include "Exception.h"

#include <cassert>
#include <cstdlib>

namespace Moses {
namespace GHKM {

Alignment ReadAlignment(const std::string &s)
{
  Alignment a;

  const std::string digits = "0123456789";

  std::string::size_type begin = s.find_first_of(digits);
  if (begin == std::string::npos) {
    // Empty word alignments are allowed
    return a;
  }

  while (true) {
    std::string::size_type end = s.find("-", begin);
    if (end == std::string::npos) {
      throw Exception("Alignment separator '-' missing");
    }
    int src = std::atoi(s.substr(begin, end-begin).c_str());

    begin = s.find_first_of(digits, end);
    if (begin == std::string::npos) {
      throw Exception("Target index missing");
    }

    end = s.find(" ", begin);
    int tgt;
    if (end == std::string::npos) {
      tgt = std::atoi(s.substr(begin).c_str());
    } else {
      tgt = std::atoi(s.substr(begin, end-begin).c_str());
    }

    a.push_back(std::make_pair(src, tgt));

    if (end == std::string::npos) {
      break;
    }
    begin = s.find_first_of(digits, end);
  }

  return a;
}

}  // namespace GHKM
}  // namespace Moses
