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

#ifndef moses_Sentence_h
#define moses_Sentence_h

#include <vector>
#include <string>
#include "Word.h"
#include "Phrase.h"
#include "InputType.h"

namespace Moses
{

class WordsRange;
class PhraseDictionary;
class TranslationOption;
class TranslationOptionCollection;
class ChartTranslationOptions;
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
  std::vector<XmlOption*> m_xmlOptions;
  std::vector <bool> m_xmlCoverageMap;

  NonTerminalSet m_defaultLabelSet;

  void ProcessPlaceholders(const std::vector< std::pair<size_t, std::string> > &placeholders);


public:
  Sentence();
  ~Sentence();

  InputTypeEnum GetType() const {
    return SentenceInput;
  }

  //! Calls Phrase::GetSubString(). Implements abstract InputType::GetSubString()
  Phrase GetSubString(const WordsRange& r) const {
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
  void GetXmlTranslationOptions(std::vector <TranslationOption*> &list) const;
  void GetXmlTranslationOptions(std::vector <TranslationOption*> &list, size_t startPos, size_t endPos) const;
  std::vector <ChartTranslationOptions*> GetXmlChartTranslationOptions() const;

  virtual int Read(std::istream& in,const std::vector<FactorType>& factorOrder);
  void Print(std::ostream& out) const;

  TranslationOptionCollection* CreateTranslationOptionCollection() const;

  virtual void CreateFromString(const std::vector<FactorType> &factorOrder
                        , const std::string &phraseString);  // , const std::string &factorDelimiter);

  const NonTerminalSet &GetLabelSet(size_t /*startPos*/, size_t /*endPos*/) const {
    return m_defaultLabelSet;
  }

};


}

#endif
