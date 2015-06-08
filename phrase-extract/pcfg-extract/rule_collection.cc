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

#include "rule_collection.h"

#include "syntax-common/pcfg.h"

#include <cmath>

namespace MosesTraining
{
namespace Syntax
{
namespace PCFG
{

void RuleCollection::Add(std::size_t lhs, const std::vector<std::size_t> &rhs)
{
  ++collection_[lhs][rhs];
}

void RuleCollection::CreatePcfg(Pcfg &pcfg)
{
  std::vector<std::size_t> key;
  for (const_iterator p = begin(); p != end(); ++p) {
    std::size_t lhs = p->first;
    const RhsCountMap &rhs_counts = p->second;
    std::size_t total = 0;
    for (RhsCountMap::const_iterator q = rhs_counts.begin();
         q != rhs_counts.end(); ++q) {
      total += q->second;
    }
    for (RhsCountMap::const_iterator q = rhs_counts.begin();
         q != rhs_counts.end(); ++q) {
      const std::vector<std::size_t> &rhs = q->first;
      std::size_t count = q->second;
      double score = std::log(static_cast<double>(count) /
                              static_cast<double>(total));
      key.clear();
      key.push_back(lhs);
      key.insert(key.end(), rhs.begin(), rhs.end());
      pcfg.Add(key, score);
    }
  }
}

}  // namespace PCFG
}  // namespace Syntax
}  // namespace MosesTraining
