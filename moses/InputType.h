// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
// vim:tabstop=2

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

#ifndef moses_InputType_h
#define moses_InputType_h

#include <string>
#include "TypeDef.h"
#include "Phrase.h"
#include "TargetPhraseCollection.h"
#include "ReorderingConstraint.h"
#include "NonTerminal.h"
#include "Range.h"
#include "parameters/AllOptions.h"

namespace Moses
{
class Factor;
class PhraseDictionary;
class TranslationOptionCollection;
class ChartTranslationOptions;
class TranslationTask;
/** base class for all types of inputs to the decoder,
 *  eg. sentences, confusion networks, lattices and tree
 */
class InputType
{
protected:
  AllOptions::ptr m_options;
  long m_translationId; 	//< contiguous Id
  long m_documentId;
  long m_topicId;
  std::string m_weightSetting;
  std::vector<std::string> m_topicIdAndProb;
  bool m_useTopicId;
  bool m_useTopicIdAndProb;
  bool m_hasMetaData;
  bool m_specifiesWeightSetting;
  long m_segId;
  ReorderingConstraint m_reorderingConstraint; /**< limits on reordering specified either by "-mp" switch or xml tags */
  std::string m_textType;
  std::string m_passthrough;
  boost::shared_ptr<std::vector<std::string> > m_context;
public:

  // used in -continue-partial-translation
  std::vector<bool> m_sourceCompleted;
  std::string m_initialTargetPhrase;
  size_t m_frontSpanCoveredLength;
  // how many words from the beginning are covered

  // Coordinates in user-defined spaces (see "coord" XML tag)
  SPTR<std::map<size_t const, std::vector<float> > > m_coordMap;

  InputType(AllOptions::ptr const& opts, long translationId = 0);
  virtual ~InputType();

  virtual InputTypeEnum GetType() const = 0;

  AllOptions::ptr const& options() const {
    return m_options;
  }

  long GetTranslationId() const {
    return m_translationId;
  }
  void SetTranslationId(long translationId) {
    m_translationId = translationId;
  }
  long GetDocumentId() const {
    return m_documentId;
  }
  void SetDocumentId(long documentId) {
    m_documentId = documentId;
  }
  long GetTopicId() const {
    return m_topicId;
  }
  void SetTopicId(long topicId) {
    m_topicId = topicId;
  }
  const std::vector<std::string>* GetTopicIdAndProb() const {
    return &m_topicIdAndProb;
  }
  void SetTopicIdAndProb(std::vector<std::string> topicIdAndProb) {
    m_topicIdAndProb = topicIdAndProb;
  }
  bool GetUseTopicId() const {
    return m_useTopicId;
  }
  void SetUseTopicId(bool useTopicId) {
    m_useTopicId = useTopicId;
  }
  bool GetUseTopicIdAndProb() const {
    return m_useTopicIdAndProb;
  }
  void SetUseTopicIdAndProb(bool useTopicIdAndProb) {
    m_useTopicIdAndProb = useTopicIdAndProb;
  }
  std::string GetTextType() const {
    return m_textType;
  }
  void SetSpecifiesWeightSetting(bool specifiesWeightSetting) {
    m_specifiesWeightSetting = specifiesWeightSetting;
  }
  bool GetSpecifiesWeightSetting() const {
    return m_specifiesWeightSetting;
  }
  void SetWeightSetting(std::string settingName) {
    m_weightSetting = settingName;
  }
  std::string GetWeightSetting() const {
    return m_weightSetting;
  }
  void SetTextType(std::string type) {
    m_textType = type;
  }
  std::string GetPassthroughInformation() const {
    return m_passthrough;
  }
  void SetPassthroughInformation(std::string &passthrough) {
    m_passthrough = passthrough;
  }
  //! returns the number of words moved
  virtual int ComputeDistortionDistance(const Range& prev, const Range& current) const;

  //! In a word lattice, tells you if there's a path from node start to node end
  virtual bool CanIGetFromAToB(size_t start, size_t end) const;

  //! is there a path covering [range] (lattice only, otherwise true)
  inline bool IsCoveragePossible(const Range& range) const {
    return CanIGetFromAToB(range.GetStartPos(), range.GetEndPos() + 1);
  }

  //! In a word lattice, you can't always get from node A to node B
  inline bool IsExtensionPossible(const Range& prev, const Range& current) const {
    //  return ComputeDistortionDistance(prev, current) < 100000;
    size_t t = prev.GetEndPos()+1;  // 2
    size_t l = current.GetEndPos()+1;   //l=1
    size_t r = l;
    if (l<t) {
      r = t;  //r=2
    } else {
      l = t;
    }
    if (!CanIGetFromAToB(l,r)) return false;

    // there's another check here: a current span may end at a place that previous could get to,
    // but it may not *START* at a place it can get to. We'll also have to check if we're going left or right

    r = current.GetStartPos();
    l = prev.GetEndPos()+1;
    if (l == r) return true;
    if (prev.GetEndPos() > current.GetStartPos()) {
      r = prev.GetStartPos();
      l = current.GetEndPos()+1;
      if (r == l) return true;
    }
    return CanIGetFromAToB(l,r);
  }

  //! number of words in this sentence/confusion network
  virtual size_t GetSize() const =0;

  virtual boost::shared_ptr<std::vector<std::string> > const&
  GetContext() const {
    return m_context;
  }

  virtual void
  SetContext(boost::shared_ptr<std::vector<std::string> > const& ctx) {
    m_context = ctx;
  }

  //! populate this InputType with data from in stream
  virtual int
  Read(std::istream& in) = 0;
  // ,
  //    std::vector<FactorType> const& factorOrder,
  //    AllOptions const& opts) =0;

  //! Output debugging info to stream out
  virtual void Print(std::ostream&) const =0;

  //! create trans options specific to this InputType
  virtual TranslationOptionCollection*
  CreateTranslationOptionCollection(ttasksptr const& ttask) const=0;

  //! return substring. Only valid for Sentence class. TODO - get rid of this fn
  virtual Phrase GetSubString(const Range&) const =0;

  //! return substring at a particular position. Only valid for Sentence class. TODO - get rid of this fn
  virtual const Word& GetWord(size_t pos) const=0;

  //! Returns the reordering constraints
  const ReorderingConstraint& GetReorderingConstraint() const {
    return m_reorderingConstraint;
  };

  virtual const NonTerminalSet &GetLabelSet(size_t startPos, size_t endPos) const = 0;

  virtual std::vector <ChartTranslationOptions*> GetXmlChartTranslationOptions() const;

  TO_STRING();

};

std::ostream& operator<<(std::ostream&,InputType const&);

}

#endif
