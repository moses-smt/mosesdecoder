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
#include <string>
#include "Sentence.h"

namespace Moses
{

/**
 * Adds a vector of strings to Sentence that are filled from tab-separated input.
 * The first column is just treated as the normal input sentence with all the XML
 * processing and stuff. Then it contains a vector of strings that contains all
 * other columns.
 *
 * At creation time calls FeatureFunction::ProcessColumns(m_columns) once for all
 * feature functions. So any feature function can do anything with any column.
 * Ideally, feature functions should keep the parse results for the columns in
 * thread-specific storage, e.g. boost::thread_specific_ptr<Something>.
 *
 * In theory a column can contain anything, even text-serialized parse trees or
 * classifier features etc.
 * 
 */

typedef std::vector<std::string> TabbedColumns;
 
class TabbedSentence : public Sentence
{

protected:


public:
  TabbedSentence() {
    std::cerr << "I am a TabbedSentence" << std::endl;
  }
  ~TabbedSentence() {}

  InputTypeEnum GetType() const {
    return TabbedSentenceInput;
  }

  // Splits off the first tab-separated column and passes it to
  // Sentence::CreateFromString(...), the remaining columns are stored in
  // m_columns .
  
  virtual void CreateFromString(const std::vector<FactorType> &factorOrder
                        , const std::string &tabbedString);
  
  virtual int Read(std::istream& in,const std::vector<FactorType>& factorOrder);

  const TabbedColumns& GetColumns() const {
    return m_columns;
  }

private:
  TabbedColumns m_columns;
  
};


}

