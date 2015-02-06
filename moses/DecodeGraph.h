// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#ifndef moses_DecodeGraph_h
#define moses_DecodeGraph_h

#include "util/exception.hh"
#include <list>
#include <iterator>
#include "TypeDef.h"

namespace Moses
{

class DecodeStep;

//! list of DecodeSteps which factorizes the translation
class DecodeGraph
{
protected:
  std::list<const DecodeStep*> m_steps;
  size_t m_id; // contiguous unique id, starting from 0
  size_t m_maxChartSpan;
  size_t m_backoff;

public:
  /**
    * position: The position of this graph within the decode sequence.
    **/
  DecodeGraph(size_t id)
    : m_id(id)
    , m_maxChartSpan(NOT_FOUND)
    , m_backoff(0) {
  }

  // for chart decoding
  DecodeGraph(size_t id, size_t maxChartSpan)
    : m_id(id)
    , m_maxChartSpan(maxChartSpan) {
  }

  //! iterators
  typedef std::list<const DecodeStep*>::iterator iterator;
  typedef std::list<const DecodeStep*>::const_iterator const_iterator;
  const_iterator begin() const {
    return m_steps.begin();
  }
  const_iterator end() const {
    return m_steps.end();
  }

  virtual ~DecodeGraph();

  //! Add another decode step to the graph
  void Add(DecodeStep *decodeStep);

  size_t GetSize() const {
    return m_steps.size();
  }

  size_t GetMaxChartSpan() const {
    UTIL_THROW_IF2(m_maxChartSpan == NOT_FOUND, "Max chart span not specified");
    return m_maxChartSpan;
  }

  size_t GetBackoff() const {
    return m_backoff;
  }

  void SetBackoff(size_t backoff) {
    m_backoff = backoff;
  }

  size_t GetId() const {
    return m_id;
  }

};


}
#endif
