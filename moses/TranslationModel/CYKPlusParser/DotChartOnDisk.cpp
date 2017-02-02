// $Id$
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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
#include <algorithm>
#include "DotChartOnDisk.h"
#include "moses/Util.h"
#include "OnDiskPt/PhraseNode.h"

using namespace std;

namespace Moses
{
DottedRuleStackOnDisk::DottedRuleStackOnDisk(size_t size)
  :m_coll(size)
{
  for (size_t ind = 0; ind < size; ++ind) {
    m_coll[ind] = new DottedRuleCollOnDisk();
  }
}

DottedRuleStackOnDisk::~DottedRuleStackOnDisk()
{
  RemoveAllInColl(m_coll);
  RemoveAllInColl(m_savedNode);
}

class SavedNodesOderer
{
public:
  bool operator()(const SavedNodeOnDisk* a, const SavedNodeOnDisk* b) const {
    bool ret = a->GetDottedRule().GetLastNode().GetCount(0) > b->GetDottedRule().GetLastNode().GetCount(0);
    return ret;
  }
};

void DottedRuleStackOnDisk::SortSavedNodes()
{
  sort(m_savedNode.begin(), m_savedNode.end(), SavedNodesOderer());
}

};
