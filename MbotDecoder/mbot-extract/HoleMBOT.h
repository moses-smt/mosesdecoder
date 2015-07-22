// modified for mbot support
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
#ifndef HOLEMBOT_H_INCLUDED_
#define HOLEMBOT_H_INCLUDED_

#include <cassert>
#include <list>
#include <string>
#include <vector>
#include <iostream>

class HoleMBOT
{
protected:
  std::vector<int> m_start, m_end, m_pos, m_fraglength;
  std::vector<std::string> m_label;
  int m_complength;

public:
  HoleMBOT()
    : m_start(2)
    , m_end(2)
    , m_pos(2)
    , m_label(2)
  {}

  HoleMBOT(const HoleMBOT &copy)
    : m_start(copy.m_start)
    , m_end(copy.m_end)
    , m_pos(copy.m_pos)
    , m_label(copy.m_label)
  	, m_fraglength(copy.m_fraglength)
    , m_complength(copy.m_complength)
  {}

  // Hole for dummy HoleCollection (target tree fragments sorted by position)
  HoleMBOT(const HoleMBOT &copy, std::vector<int>::size_type i)
    : m_start(2)
  	, m_end(2)
    , m_pos(2)
    , m_label(2) {
    m_start[0] = copy.m_start[0];
    m_start[1] = copy.m_start[i];
    m_end[0] = copy.m_end[0];
    m_end[1] = copy.m_end[i];
    m_pos[0] = copy.m_pos[0];
    m_pos[1] = copy.m_pos[i];
    m_label[0] = copy.m_label[0];
    m_label[1] = copy.m_label[i];
  }

  HoleMBOT(int startS, int endS, std::vector<std::pair<int,int> > spanT)
   : m_start(1)
    , m_end(1)
    , m_pos(spanT.size()+1)
    , m_label(spanT.size()+1) {
          m_start[0] = startS;
          m_end[0] = endS;
          int tmpSpan = 0;
          for (std::vector<int>::size_type i = 0; i<spanT.size(); ++i) {
                m_start.push_back(spanT[i].first);
                m_end.push_back(spanT[i].second);
                // save length of each target tree fragment
                m_fraglength.push_back(spanT[i].second - spanT[i].first + 1);
                tmpSpan += spanT[i].second - spanT[i].first + 1;
          }
          // save complete length of target tree fragments
          m_complength = tmpSpan;
   }

   // for mbot
  int GetStartS() const {
    return m_start[0];
  }

  // for mbot (multiple target starts)
  std::vector<int> GetStartT() const {
    std::vector<int> values;
    for (std::vector<int>::size_type i=1; i<m_start.size(); ++i) {
      values.push_back(m_start[i]);
    }
    return values;
  }

  int GetStart(size_t direction) const {
    return m_start[direction];
  }

  // for mbot
  int GetEndS() const {
    return m_end[0];
  }

  // for mbot
  std::vector<int> GetEndT() const {
    std::vector<int> values;
    for (std::vector<int>::size_type i=1; i<m_end.size(); ++i) {
      values.push_back(m_end[i]);
    }
    return values;
  }

  // for mbot
  std::vector<int> GetFragmentsLength() const {
    std::vector<int> values;
    for (std::vector<int>::size_type i=0; i<m_fraglength.size(); ++i) {
      values.push_back(m_fraglength[i]);
    }
    return values;
  }

  int GetEnd(size_t direction) const {
    return m_end[direction];
  }

  void SetPosS(int pos) {
    m_pos[0] = pos;
  }

  void SetPosT(int pos, int dir) {
    m_pos[dir] = pos;
  }

  void SetPos(int pos, size_t direction) {
	m_pos[direction] = pos;
  }
 
  // for mbot
  int GetPosS() const {
    return m_pos[0];
  }

  // for mbot
  std::vector<int> GetPosT() const {
    std::vector<int> posT;
    for (std::vector<int>::size_type i=1; i<m_pos.size(); ++i)
      posT.push_back(m_pos[i]);
    return posT;
  }

  int GetPos(size_t direction) const {
	return m_pos[direction];
  }

  void SetLabelS(const std::string &label) {
    m_label[0] = label;
  }

  void SetLabelT(const std::string &label, int dir) {
    m_label[dir] = label;
  }

  // for mbot
  const std::string &GetLabelS() const {
    return m_label[0];
  }

  // for mbot
  std::vector<std::string> GetLabelT() const {
    std::vector<std::string> labels;
    for (std::vector<int>::size_type i=1; i<m_label.size(); ++i) {
      labels.push_back(m_label[i]);
    }
    return labels;
  }

  const std::string &GetLabel(size_t direction) {
	  return m_label[direction];
  }

  std::vector< std::pair<int,int> > GetPairedSpans() const {
	  std::vector<int> start = GetStartT();
	  std::vector<int> end = GetEndT();
	  std::vector<std::pair<int,int> > paired;

	  for (int i=0; i<start.size(); ++i) {
		  paired.push_back(std::make_pair(start[i], end[i]));
	  }
	  return paired;
  }

  const int &GetCompLength() const {
	  return m_complength;
  }

  bool Overlap(const HoleMBOT &otherHole) const {
	  std::vector<int> otherEnd = otherHole.GetEndT();
	  std::vector<int> otherStart = otherHole.GetStartT();
	  std::vector<int> thisStart = GetStartT();
	  std::vector<int> thisEnd = GetEndT();

	  for (int i=0; i<otherEnd.size(); ++i) {
		  for (int j=0; j<thisStart.size(); ++j) {
			  if ( ! ( otherEnd[i] < thisStart[j] || otherStart[i] > thisEnd[j] ) )
				  return true;
          }
	  }
	  return false;
  } 

  bool subTargetFragments(std::vector< std::pair <int,int> > targetSpans) const {
	std::vector<int> thisStart = GetStartT();
	std::vector<int> thisEnd = GetEndT();
	bool subFrag;
	for (int i=0; i<thisStart.size(); ++i) {
		subFrag = false;
		for (int j=0; j<targetSpans.size(); ++j) {
			if ( thisStart[i] >= targetSpans[j].first && thisEnd[i] <= targetSpans[j].second ) {
				subFrag = true;
				break;
			}
           }
           if (!subFrag ) {
        	   return false;
           }
         }
         return true;
  }

  bool NeighborSource(const HoleMBOT &otherHole) const {
    return ( otherHole.GetEnd(0)+1 == GetStart(0) ||
    otherHole.GetStart(0) == GetEnd(0)+1 );
  }

};

typedef std::list<HoleMBOT> HoleList;

class HoleTargetOrderer
{
public:
  bool operator()(const HoleMBOT holeA, const HoleMBOT holeB) const {
    //assert(holeA->GetStart(0) != holeB->GetStart(0));
    assert(holeA.GetStart(1) != holeB.GetStart(1));
    //return holeA->GetStart(0) < holeB->GetStart(0);
    return holeA.GetStart(1) < holeB.GetStart(1);
  }
};


#endif
