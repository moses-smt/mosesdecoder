// $Id$
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

#pragma once

#include <vector>
#include <set>

namespace MosesTraining
{

class WordsRange;

class AlignmentElement
{
protected:
  std::set<size_t> m_elements;
public:
  typedef std::set<size_t>::iterator iterator;
  typedef std::set<size_t>::const_iterator const_iterator;
  const_iterator begin() const {
    return m_elements.begin();
  }
  const_iterator end() const {
    return m_elements.end();
  }

  AlignmentElement() {
  }

  size_t GetSize() const {
    return m_elements.size();
  }

  void Merge(size_t align);
};

class AlignmentPhrase
{
protected:
  std::vector<AlignmentElement> m_elements;
public:
  AlignmentPhrase(size_t size)
    :m_elements(size) {
  }
  void Merge(const AlignmentPhrase &newAlignment, const WordsRange &newAlignmentRange);
  void Merge(const std::vector< std::vector<size_t> > &source);
  size_t GetSize() const {
    return m_elements.size();
  }
  const AlignmentElement &GetElement(size_t pos) const {
    return m_elements[pos];
  }
};

} // namespace

