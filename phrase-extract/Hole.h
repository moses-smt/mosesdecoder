/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2010 University of Edinburgh

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
#ifndef HOLE_H_INCLUDED_
#define HOLE_H_INCLUDED_

#include <cassert>
#include <list>
#include <string>
#include <vector>

namespace MosesTraining
{

class Hole
{
protected:
  std::vector<int> m_start, m_end, m_pos;
  std::vector<std::string> m_label;

public:
  Hole()
    : m_start(2)
    , m_end(2)
    , m_pos(2)
    , m_label(2) {
  }

  Hole(const Hole &copy)
    : m_start(copy.m_start)
    , m_end(copy.m_end)
    , m_pos(copy.m_pos)
    , m_label(copy.m_label) {
  }

  Hole(int startS, int endS, int startT, int endT)
    : m_start(2)
    , m_end(2)
    , m_pos(2)
    , m_label(2) {
    m_start[0] = startS;
    m_end[0] = endS;
    m_start[1] = startT;
    m_end[1] = endT;
  }

  int GetStart(size_t direction) const {
    return m_start[direction];
  }

  int GetEnd(size_t direction) const {
    return m_end[direction];
  }

  int GetSize(size_t direction) const {
    return m_end[direction] - m_start[direction] + 1;
  }

  void SetPos(int pos, size_t direction) {
    m_pos[direction] = pos;
  }

  int GetPos(size_t direction) const {
    return m_pos[direction];
  }

  void SetLabel(const std::string &label, size_t direction) {
    m_label[direction] = label;
  }

  const std::string &GetLabel(size_t direction) const {
    return m_label[direction];
  }

  bool Overlap(const Hole &otherHole, size_t direction) const {
    return ! ( otherHole.GetEnd(direction)   < GetStart(direction) ||
               otherHole.GetStart(direction) > GetEnd(direction) );
  }

  bool Neighbor(const Hole &otherHole, size_t direction) const {
    return ( otherHole.GetEnd(direction)+1 == GetStart(direction) ||
             otherHole.GetStart(direction) == GetEnd(direction)+1 );
  }
};

typedef std::list<Hole> HoleList;

class HoleSourceOrderer
{
public:
  bool operator()(const Hole* holeA, const Hole* holeB) const {
    assert(holeA->GetStart(0) != holeB->GetStart(0));
    return holeA->GetStart(0) < holeB->GetStart(0);
  }
};

}

#endif
