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

#include "pcfg.h"

#include "exception.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <cassert>

namespace Moses {
namespace PCFG {

void Pcfg::Add(const Key &key, double score) {
  rules_[key] = score;
}

bool Pcfg::Lookup(const Key &key, double &score) const {
  Map::const_iterator p = rules_.find(key);
  if (p == rules_.end()) {
    return false;
  }
  score = p->second;
  return true;
}

void Pcfg::Read(std::istream &input, Vocabulary &vocab) {
  std::string line;
  std::string lhs_string;
  std::vector<std::string> rhs_strings;
  std::string score_string;
  Key key;
  while (std::getline(input, line)) {
    // Read LHS.
    std::size_t pos = line.find("|||");
    if (pos == std::string::npos) {
      throw Exception("missing first delimiter");
    }
    lhs_string = line.substr(0, pos);
    boost::trim(lhs_string);

    // Read RHS.
    std::size_t begin = pos+3;
    pos = line.find("|||", begin);
    if (pos == std::string::npos) {
      throw Exception("missing second delimiter");
    }
    std::string rhs_text = line.substr(begin, pos-begin);
    boost::trim(rhs_text);
    rhs_strings.clear();
    boost::split(rhs_strings, rhs_text, boost::algorithm::is_space(),
                 boost::algorithm::token_compress_on);

    // Read score.
    score_string = line.substr(pos+3);
    boost::trim(score_string);

    // Construct key.
    key.clear();
    key.reserve(rhs_strings.size()+1);
    key.push_back(vocab.Insert(lhs_string));
    for (std::vector<std::string>::const_iterator p = rhs_strings.begin();
         p != rhs_strings.end(); ++p) {
      key.push_back(vocab.Insert(*p));
    }

    // Add rule.
    double score = boost::lexical_cast<double>(score_string);
    Add(key, score);
  }
}

void Pcfg::Write(const Vocabulary &vocab, std::ostream &output) const {
  for (const_iterator p = begin(); p != end(); ++p) {
    const Key &key = p->first;
    double score = p->second;
    std::vector<std::size_t>::const_iterator q = key.begin();
    std::vector<std::size_t>::const_iterator end = key.end();
    output << vocab.Lookup(*q++) << " |||";
    while (q != end) {
      output << " " << vocab.Lookup(*q++);
    }
    output << " ||| " << score << std::endl;
  }
}

}  // namespace PCFG
}  // namespace Moses
