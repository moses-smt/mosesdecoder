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
#include "moses/InputPath.h"

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

  virtual ~PhraseDictionary() {
  }

  virtual void Load() = 0;

  //! table limit number.
  size_t GetTableLimit() const {
    return m_tableLimit;
  }

  // LEGACY - The preferred method is to override GetTargetPhraseCollectionBatch().
  // See class PhraseDictionaryMemory or PhraseDictionaryOnDisk for details
  //! find list of translations that can translates src. Only for phrase input
  virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const;
  //! find list of translations that can translates a portion of src. Used by confusion network decoding
  virtual const TargetPhraseCollection *GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const;

  virtual void GetTargetPhraseCollectionBatch(const InputPathList &phraseDictionaryQueue) const;

  //! Create entry for translation of source to targetPhrase
  virtual void InitializeForInput(InputType const& source) {
  }
  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing(const InputType& source) {
  }

  //! Create a sentence-specific manager for SCFG rule lookup.
  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const InputType &,
    const ChartCellCollectionBase &) = 0;

  const std::string &GetFilePath() const {
    return m_filePath;
  }

  const std::vector<FeatureFunction*> &GetFeaturesToApply() const {
    return m_featuresToApply;
  }

  void SetParameter(const std::string& key, const std::string& value);


protected:
  size_t m_tableLimit;
  std::string m_filePath;

  // features to apply evaluate target phrase when loading.
  // NOT when creating translation options. Those are in DecodeStep
  std::vector<FeatureFunction*> m_featuresToApply;

  // MUST be called at the start of Load()
  void SetFeaturesToApply();
};

}
#endif
