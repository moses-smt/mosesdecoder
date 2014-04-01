// $Id: RuleCubeQueueMBOT.cpp,v 1.1.1.1 2013/01/06 16:54:17 braunefe Exp $
// vim:tabstop=2
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

#include "RuleCubeQueueMBOT.h"

#include "RuleCubeItem.h"
#include "StaticData.h"

namespace Moses
{

RuleCubeQueueMBOT::~RuleCubeQueueMBOT()
{
  while (!m_mbotQueue.empty()) {
    RuleCubeMBOT *cube = m_mbotQueue.top();
    m_mbotQueue.pop();
    delete cube;
  }
}

void RuleCubeQueueMBOT::Add(RuleCubeMBOT *ruleCube)
{
  m_mbotQueue.push(ruleCube);
}

ChartHypothesisMBOT *RuleCubeQueueMBOT::PopMBOT()
{
  // pop the most promising rule cube
  RuleCubeMBOT *cube = m_mbotQueue.top();
  m_mbotQueue.pop();

  // pop the most promising item from the cube and get the corresponding
  // hypothesis
  RuleCubeItemMBOT *item = cube->PopMBOT(m_manager);
  if (StaticData::Instance().GetCubePruningLazyScoring()) {
    item->CreateHypothesis(cube->GetTranslationOptionMBOT(), m_manager);
  }
  ChartHypothesisMBOT *hypo = item->ReleaseHypothesisMBOT();

  // if the cube contains more items then push it back onto the queue
  if (!cube->IsEmptyMBOT()) {
    m_mbotQueue.push(cube);
  } else {
    delete cube;
  }
  return hypo;
}

}
