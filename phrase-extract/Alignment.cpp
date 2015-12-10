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

#include "phrase-extract/syntax-common/exception.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>

namespace MosesTraining
{

void ReadAlignment(const std::string &s, Alignment &a)
{
  const std::string digits = "0123456789";

  a.clear();

  std::string::size_type begin = 0;
  while (true) {
    std::string::size_type end = s.find("-", begin);
    if (end == std::string::npos) {
      return;
    }
    int src = std::atoi(s.substr(begin, end-begin).c_str());
    if (end+1 == s.size()) {
      throw Syntax::Exception("Target index missing");
    }

    begin = end+1;
    end = s.find_first_not_of(digits, begin+1);
    int tgt;
    if (end == std::string::npos) {
      tgt = std::atoi(s.substr(begin).c_str());
      a.push_back(std::make_pair(src, tgt));
      return;
    } else {
      tgt = std::atoi(s.substr(begin, end-begin).c_str());
      a.push_back(std::make_pair(src, tgt));
    }
    begin = end+1;
  }
}

void FlipAlignment(Alignment &a)
{
  for (Alignment::iterator p = a.begin(); p != a.end(); ++p) {
    std::swap(p->first, p->second);
  }
}

}  // namespace MosesTraining
