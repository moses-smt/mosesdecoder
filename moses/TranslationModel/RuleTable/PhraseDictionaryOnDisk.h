// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include <map>
#include <vector>
#include <string>
#include "moses/TranslationModel/PhraseDictionary.h"
#include "OnDiskPt/OnDiskWrapper.h"
#include "OnDiskPt/Word.h"
#include "OnDiskPt/PhraseNode.h"

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#else
#include <boost/scoped_ptr.hpp>
#endif

namespace Moses
{
class TargetPhraseCollection;
class DottedRuleStackOnDisk;
class InputPath;
class ChartParser;

/** Implementation of on-disk phrase table for hierarchical/syntax model.
 */
class PhraseDictionaryOnDisk : public PhraseDictionary
{
  typedef PhraseDictionary MyBase;
  friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryOnDisk&);
  friend class ChartRuleLookupManagerOnDisk;

protected:
#ifdef WITH_THREADS
  boost::thread_specific_ptr<OnDiskPt::OnDiskWrapper> m_implementation;
#else
  boost::scoped_ptr<OnDiskPt::OnDiskWrapper> m_implementation;
#endif

  size_t m_maxSpanDefault, m_maxSpanLabelled;

  OnDiskPt::OnDiskWrapper &GetImplementation();
  const OnDiskPt::OnDiskWrapper &GetImplementation() const;

  void GetTargetPhraseCollectionBatch(InputPath &inputPath) const;

  Moses::TargetPhrase *ConvertToMoses(const OnDiskPt::TargetPhrase &targetPhraseOnDisk
                                      , const std::vector<Moses::FactorType> &inputFactors
                                      , const std::vector<Moses::FactorType> &outputFactors
                                      , const OnDiskPt::Vocab &vocab
                                      , const Moses::PhraseDictionary &phraseDict
                                      , const std::vector<float> &weightT
                                      , bool isSyntax) const;

  void ConvertToMoses(const OnDiskPt::Word &wordOnDisk,
                      const std::vector<Moses::FactorType> &outputFactorsVec,
                      const OnDiskPt::Vocab &vocab,
                      Moses::Word &overwrite) const;

public:
  PhraseDictionaryOnDisk(const std::string &line);
  ~PhraseDictionaryOnDisk();
  void Load(AllOptions::ptr const& opts);

  // PhraseDictionary impl
  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &parser,
    const ChartCellCollectionBase &,
    std::size_t);

  virtual void InitializeForInput(ttasksptr const& ttask);
  void GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollection(const OnDiskPt::PhraseNode *ptNode) const;

  TargetPhraseCollection::shared_ptr
  GetTargetPhraseCollectionNonCache(const OnDiskPt::PhraseNode *ptNode) const;

  Moses::TargetPhraseCollection::shared_ptr
  ConvertToMoses(
    const OnDiskPt::TargetPhraseCollection::shared_ptr targetPhrasesOnDisk
    , const std::vector<Moses::FactorType> &inputFactors
    , const std::vector<Moses::FactorType> &outputFactors
    , const Moses::PhraseDictionary &phraseDict
    , const std::vector<float> &weightT
    , OnDiskPt::Vocab &vocab
    , bool isSyntax) const;

  OnDiskPt::Word *ConvertFromMoses(OnDiskPt::OnDiskWrapper &wrapper, const std::vector<Moses::FactorType> &factorsVec
                                   , const Moses::Word &origWord) const;

  void SetParameter(const std::string& key, const std::string& value);

};

}  // namespace Moses
