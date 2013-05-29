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

#ifndef moses_PhraseDictionary_h
#define moses_PhraseDictionary_h

#include <iostream>
#include <map>
#include <memory>
#include <list>
#include <stdexcept>
#include <vector>
#include <string>

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#endif

#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/DecodeFeature.h"

namespace Moses
{

class StaticData;
class InputType;
class WordsRange;
class ChartCellCollectionBase;
class ChartRuleLookupManager;

/**
  * Abstract base class for phrase dictionaries (tables).
  **/
class PhraseDictionary :  public DecodeFeature
{
public:
  PhraseDictionary(const std::string &description, const std::string &line);

  virtual ~PhraseDictionary()
  {}

  //! table limit number.
  size_t GetTableLimit() const {
    return m_tableLimit;
  }

  //! find list of translations that can translates src. Only for phrase input
  virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const=0;
  //! find list of translations that can translates a portion of src. Used by confusion network decoding
  virtual const TargetPhraseCollection *GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const;

  //! Create entry for translation of source to targetPhrase
  virtual void InitializeForInput(InputType const& source)
  {}
  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing(const InputType& source)
  {}

  //! Create a sentence-specific manager for SCFG rule lookup.
  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &) = 0;

  //Initialises the dictionary (may involve loading from file)
  virtual bool InitDictionary() = 0;

  //Get the dictionary. Be sure to initialise it first.
  const PhraseDictionary* GetDictionary() const;
  PhraseDictionary* GetDictionary();

  const std::string &GetFilePath() const {
    return m_filePath;
  }

protected:
  size_t m_tableLimit;


  unsigned m_numInputScores;
  std::string m_filePath;

  std::string m_targetFile;
  std::string m_alignmentsFile;
};

}
#endif
