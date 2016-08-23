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

#ifndef MERT_FEATURE_DATA_ITERATOR_H_
#define MERT_FEATURE_DATA_ITERATOR_H_

/**
  * For loading from the feature data file.
**/

#include <fstream>
#include <map>
#include <stdexcept>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/shared_ptr.hpp>

#include "util/exception.hh"
#include "util/string_piece.hh"

#include "FeatureStats.h"

namespace util
{
class FilePiece;
}

namespace MosesTuning
{


class FileFormatException : public util::Exception
{
public:
  explicit FileFormatException(const std::string& filename, const std::string& line) {
    *this << "Error in line \"" << line << "\" of " << filename;
  }
};


/** Assumes a delimiter, so only apply to tokens */
int ParseInt(const StringPiece& str );

/** Assumes a delimiter, so only apply to tokens */
float ParseFloat(const StringPiece& str);


class FeatureDataItem
{
public:
  std::vector<float> dense;
  SparseVector sparse;
};

bool operator==(FeatureDataItem const& item1, FeatureDataItem const& item2);
std::size_t hash_value(FeatureDataItem const& item);

class FeatureDataIterator :
  public boost::iterator_facade<FeatureDataIterator,
  const std::vector<FeatureDataItem>,
  boost::forward_traversal_tag>
{
public:
  FeatureDataIterator();
  explicit FeatureDataIterator(const std::string& filename);
  ~FeatureDataIterator();

  static FeatureDataIterator end() {
    return FeatureDataIterator();
  }


private:
  friend class boost::iterator_core_access;

  void increment();
  bool equal(const FeatureDataIterator& rhs) const;
  const std::vector<FeatureDataItem>& dereference() const;

  void readNext();

  boost::shared_ptr<util::FilePiece> m_in;
  std::vector<FeatureDataItem> m_next;
};

}

#endif  // MERT_FEATURE_DATA_ITERATOR_H_
