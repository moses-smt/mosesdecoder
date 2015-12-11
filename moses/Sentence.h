// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef moses_Sentence_h
#define moses_Sentence_h

#include <vector>
#include <string>
#include "Word.h"
#include "Phrase.h"
#include "InputType.h"
#include "parameters/AllOptions.h"

namespace Moses
{

class Range;
class PhraseDictionary;
class TranslationOption;
class TranslationOptionCollection;
class ChartTranslationOptions;
class TranslationTask;
struct XmlOption;


/**
 * A Phrase class with an ID. Used specifically as source input so contains functionality to read
 *	from IODevice and create trans opt
 */
class Sentence : public Phrase, public InputType
{
protected:

  /**
   * Utility method that takes in a string representing an XML tag and the name of the attribute,
   * and returns the value of that tag if present, empty string otherwise
   */
  std::vector<XmlOption const*> m_xmlOptions;
  std::vector <bool> m_xmlCoverageMap;

  NonTerminalSet m_defaultLabelSet;

  void ProcessPlaceholders(const std::vector< std::pair<size_t, std::string> > &placeholders);

  // "Document Level Translation" instructions, see aux_interpret_dlt
  std::vector<std::map<std::string,std::string> > m_dlt_meta;

public:
  Sentence(AllOptions::ptr const& opts);
  Sentence(AllOptions::ptr const& opts, size_t const transId, std::string stext);
  // std::vector<FactorType> const* IFO = NULL);
  // Sentence(size_t const transId, std::string const& stext);
  ~Sentence();

  InputTypeEnum GetType() const {
    return SentenceInput;
  }

  //! Calls Phrase::GetSubString(). Implements abstract InputType::GetSubString()
  Phrase GetSubString(const Range& r) const {
    return Phrase::GetSubString(r);
  }

  //! Calls Phrase::GetWord(). Implements abstract InputType::GetWord()
  const Word& GetWord(size_t pos) const {
    return Phrase::GetWord(pos);
  }

  //! Calls Phrase::GetSize(). Implements abstract InputType::GetSize()
  size_t GetSize() const {
    return Phrase::GetSize();
  }

  //! Returns true if there were any XML tags parsed that at least partially covered the range passed
  bool XmlOverlap(size_t startPos, size_t endPos) const;

  //! populates vector argument with XML force translation options for the specific range passed
  void GetXmlTranslationOptions(std::vector<TranslationOption*> &list) const;
  void GetXmlTranslationOptions(std::vector<TranslationOption*> &list, size_t startPos, size_t endPos) const;
  std::vector<ChartTranslationOptions*> GetXmlChartTranslationOptions() const;

  virtual int
  Read(std::istream& in);
  // , const std::vector<FactorType>& factorOrder, AllOptions const& opts);

  void Print(std::ostream& out) const;

  TranslationOptionCollection*
  CreateTranslationOptionCollection(ttasksptr const& ttask) const;

  virtual void
  CreateFromString(std::vector<FactorType> const &factorOrder,
                   std::string const& phraseString);

  const NonTerminalSet&
  GetLabelSet(size_t /*startPos*/, size_t /*endPos*/) const {
    return m_defaultLabelSet;
  }


  void init(std::string line);

  std::vector<std::map<std::string,std::string> > const&
  GetDltMeta() const {
    return m_dlt_meta;
  }

private:
  // auxliliary functions for Sentence initialization
  // void aux_interpret_sgml_markup(std::string& line);
  // void aux_interpret_dlt(std::string& line);
  // void aux_interpret_xml (std::string& line, std::vector<size_t> & xmlWalls,
  // 			    std::vector<std::pair<size_t, std::string> >& placeholders);

  void
  aux_interpret_sgml_markup(std::string& line);

  void
  aux_interpret_dlt(std::string& line);

  void
  aux_interpret_xml
  (std::string& line, std::vector<size_t> & xmlWalls,
   std::vector<std::pair<size_t, std::string> >& placeholders);

  void
  aux_init_partial_translation(std::string& line);

};


}

#endif
