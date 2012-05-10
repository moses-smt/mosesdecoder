// $Id$
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

#include "RuleCubeQueue.h"

#include "RuleCubeItem.h"
#include "StaticData.h"

namespace Moses
{

RuleCubeQueue::~RuleCubeQueue()
{
  while (!m_queue.empty()) {
    RuleCube *cube = m_queue.top();
    m_queue.pop();
    delete cube;
  }
}

void RuleCubeQueue::Add(RuleCube *ruleCube)
{
  m_queue.push(ruleCube);
}

ChartHypothesis *RuleCubeQueue::Pop()
{
  // pop the most promising rule cube
  RuleCube *cube = m_queue.top();
  m_queue.pop();

  // pop the most promising item from the cube and get the corresponding
  // hypothesis
  RuleCubeItem *item = cube->Pop(m_manager);
  if (StaticData::Instance().GetCubePruningLazyScoring()) {
    item->CreateHypothesis(cube->GetTranslationOption(), m_manager);
  }
  ChartHypothesis *hypo = item->ReleaseHypothesis();

  // if the cube contains more items then push it back onto the queue
  if (!cube->IsEmpty()) {
    m_queue.push(cube);
  } else {
    delete cube;
  }

  return hypo;
}

}
