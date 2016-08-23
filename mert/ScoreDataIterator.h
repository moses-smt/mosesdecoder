/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011- University of Edinburgh

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

#ifndef MERT_SCORE_DATA_ITERATOR_H_
#define MERT_SCORE_DATA_ITERATOR_H_

/*
 * For loading from the score data file.
**/
#include <vector>


#include <boost/iterator/iterator_facade.hpp>
#include <boost/shared_ptr.hpp>

#include "util/string_piece.hh"

#include "FeatureDataIterator.h"

namespace util
{
class FilePiece;
}

namespace MosesTuning
{


typedef std::vector<float> ScoreDataItem;

class ScoreDataIterator :
  public boost::iterator_facade<ScoreDataIterator,
  const std::vector<ScoreDataItem>,
  boost::forward_traversal_tag>
{
public:
  ScoreDataIterator();
  explicit ScoreDataIterator(const std::string& filename);

  ~ScoreDataIterator();

  static ScoreDataIterator end() {
    return ScoreDataIterator();
  }

private:
  friend class boost::iterator_core_access;

  void increment();
  bool equal(const ScoreDataIterator& rhs) const;
  const std::vector<ScoreDataItem>& dereference() const;

  void readNext();

  boost::shared_ptr<util::FilePiece> m_in;
  std::vector<ScoreDataItem> m_next;
};

}


#endif  // MERT_SCORE_DATA_ITERATOR_H_
