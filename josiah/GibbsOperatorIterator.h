// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2009 University of Edinburgh
 
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

#include "Gibbler.h"

namespace Josiah {
  
  
  class FlipOperator;
  
  class GibbsOperatorIterator {
  public:
    GibbsOperatorIterator() : m_size(NOT_FOUND) {}
    virtual ~GibbsOperatorIterator() {}
    virtual bool isStartScan() = 0;
    virtual  void init(const Sample& sample) { m_size = sample.GetSourceSize();}
    virtual void next() = 0;
    virtual bool keepGoing() = 0;
    virtual void reset() {
      m_size = NOT_FOUND;
    } 
    size_t GetInputSize() { return m_size;}
  protected:
    size_t  m_size; //Size of input
  };
  
  class SwapIterator : public GibbsOperatorIterator {
  public:
    SwapIterator() : m_curPos(NOT_FOUND), m_nextHypo(NULL), GibbsOperatorIterator() {}
    virtual ~SwapIterator() {}
    virtual bool isStartScan() { return (m_curPos == NOT_FOUND && m_size == NOT_FOUND); } 
    virtual void next() { m_curPos = m_nextHypo->GetCurrSourceWordsRange().GetStartPos();}
    virtual bool keepGoing() {
      if (m_size == NOT_FOUND || m_nextHypo != NULL)
        return true;
      return false;
    }
    virtual void reset() {
      m_curPos = NOT_FOUND;
      m_nextHypo = NULL;
      GibbsOperatorIterator::reset();
    }
    size_t GetCurPos() { return m_curPos;}
    virtual  void init(const Sample& sample) { m_curPos++; m_nextHypo = NULL; m_size = sample.GetSourceSize();}
    void SetNextHypo(Hypothesis* hyp) { m_nextHypo = hyp; }
  private:
    size_t  m_curPos;
    Hypothesis* m_nextHypo;
  };
  
  class MergeSplitIterator : public GibbsOperatorIterator {
  public:
    MergeSplitIterator() : m_curPos(NOT_FOUND), GibbsOperatorIterator() {}
    virtual ~MergeSplitIterator() {}
    virtual bool isStartScan() { return (m_curPos == NOT_FOUND && m_size == NOT_FOUND); } 
    virtual void next() { m_curPos++;}
    virtual bool keepGoing() {
      if (m_size == NOT_FOUND || m_curPos < m_size -1)
        return true;
      return false;
    }
    virtual void reset() {
      m_curPos = NOT_FOUND;
      GibbsOperatorIterator::reset();
    }
    size_t GetCurPos() { return m_curPos;}
    
  private:
    size_t  m_curPos;
  };
  
  class FlipIterator : public GibbsOperatorIterator {
  public:
    FlipIterator(FlipOperator* op) : m_thisPos(NOT_FOUND), m_thatPos(NOT_FOUND), m_operator(op), m_direction(1), GibbsOperatorIterator() {}
    virtual ~FlipIterator() {}
    virtual bool isStartScan() { return (m_thisPos == NOT_FOUND && m_thatPos == NOT_FOUND && m_size == NOT_FOUND); } 
    virtual void next() ;         
    virtual bool keepGoing() ;
    virtual void reset() {
      m_thisPos = NOT_FOUND;
      m_thatPos = NOT_FOUND;
      m_direction = 1;
      GibbsOperatorIterator::reset();
    }
    int GetDirection() {
      return m_direction;
    }
    size_t GetThisPos() { return m_thisPos;}    
    size_t GetThatPos() { return m_thatPos;}   
  private:
    size_t  m_thisPos;
    size_t  m_thatPos;
    int m_direction;
    FlipOperator* m_operator;
  };  
}

