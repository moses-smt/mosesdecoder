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

#include <vector>
#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>

#include "TabbedSentence.h"

namespace Moses
{

void TabbedSentence::CreateFromString(const std::vector<FactorType> &factorOrder
                                      , const std::string &tabbedString)
{
  TabbedColumns allColumns;

  boost::split(allColumns, tabbedString, boost::is_any_of("\t"));

  if(allColumns.size() < 2) {
    Sentence::CreateFromString(factorOrder, tabbedString);
  } else {
    m_columns.resize(allColumns.size() - 1);
    std::copy(allColumns.begin() + 1, allColumns.end(), m_columns.begin());
    Sentence::CreateFromString(factorOrder, allColumns[0]);
  }
}

int TabbedSentence::Read(std::istream& in, const std::vector<FactorType>& factorOrder)
{
  TabbedColumns allColumns;

  std::string line;
  if (getline(in, line, '\n').eof())
    return 0;

  boost::split(allColumns, line, boost::is_any_of("\t"));

  if(allColumns.size() < 2) {
    std::stringstream dummyStream;
    dummyStream << line << std::endl;
    return Sentence::Read(dummyStream, factorOrder);
  } else {
    m_columns.resize(allColumns.size() - 1);
    std::copy(allColumns.begin() + 1, allColumns.end(), m_columns.begin());

    std::stringstream dummyStream;
    dummyStream << allColumns[0] << std::endl;
    return Sentence::Read(dummyStream, factorOrder);
  }
}

}
