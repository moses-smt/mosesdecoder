// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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

#ifndef moses_Hypothesis_h
#define moses_Hypothesis_h

#include <iostream>
#include <memory>

#include <boost/scoped_ptr.hpp>

#include <vector>
#include "Phrase.h"
#include "TypeDef.h"
#include "Bitmap.h"
#include "Sentence.h"
#include "Phrase.h"
#include "GenerationDictionary.h"
#include "ScoreComponentCollection.h"
#include "InputType.h"
#include "ObjectPool.h"
#include "xmlrpc-c.h"

namespace Moses
{

class SquareMatrix;
class StaticData;
class TranslationOption;
class Range;
class Hypothesis;
class FFState;
class StatelessFeatureFunction;
class StatefulFeatureFunction;
class Manager;
struct ReportingOptions;

typedef std::vector<Hypothesis*> ArcList;

/** Used to store a state in the beam search
    for the best translation. With its link back to the previous hypothesis
    m_prevHypo, we can trace back to the sentence start to read of the
    (partial) translation to this point.

		The expansion of hypotheses is handled in the class Manager, which
    stores active hypothesis in the search in hypothesis stacks.
***/
class Hypothesis
{
  friend std::ostream& operator<<(std::ostream&, const Hypothesis&);
protected:
  const Hypothesis* m_prevHypo; /*! backpointer to previous hypothesis (from which this one was created) */
  const Bitmap	&m_sourceCompleted; /*! keeps track of which words have been translated so far */
  InputType const&  m_sourceInput;
  Range				m_currSourceWordsRange; /*! source word positions of the last phrase that was used to create this hypothesis */
  Range        m_currTargetWordsRange; /*! target word positions of the last phrase that was used to create this hypothesis */
  bool							m_wordDeleted;
  float							m_futureScore;  /*! score so far */
  float							m_estimatedScore; /*! estimated future cost to translate rest of sentence */
  /*! sum of scores of this hypothesis, and previous hypotheses. Lazily initialised.  */
  mutable boost::scoped_ptr<ScoreComponentCollection> m_scoreBreakdown;
  ScoreComponentCollection m_currScoreBreakdown; /*! scores for this hypothesis only */
  std::vector<const FFState*> m_ffStates;
  const Hypothesis 	*m_winningHypo;
  ArcList 					*m_arcList; /*! all arcs that end at the same trellis point as this hypothesis */
  const TranslationOption &m_transOpt;
  Manager& m_manager;

  int m_id; /*! numeric ID of this hypothesis, used for logging */

public:
  /*! used by initial seeding of the translation process */
  Hypothesis(Manager& manager, InputType const& source, const TranslationOption &initialTransOpt, const Bitmap &bitmap, int id);
  /*! used when creating a new hypothesis using a translation option (phrase translation) */
  Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt, const Bitmap &bitmap, int id);
  ~Hypothesis();

  void PrintHypothesis() const;

  const InputType& GetInput() const {
    return m_sourceInput;
  }

  /** return target phrase used to create this hypothesis */
//	const Phrase &GetCurrTargetPhrase() const
  const TargetPhrase &GetCurrTargetPhrase() const;

  /** return input positions covered by the translation option (phrasal translation) used to create this hypothesis */
  inline const Range &GetCurrSourceWordsRange() const {
    return m_currSourceWordsRange;
  }

  inline const Range &GetCurrTargetWordsRange() const {
    return m_currTargetWordsRange;
  }

  Manager& GetManager() const {
    return m_manager;
  }

  /** output length of the translation option used to create this hypothesis */
  inline size_t GetCurrTargetLength() const {
    return m_currTargetWordsRange.GetNumWordsCovered();
  }

  void EvaluateWhenApplied(float estimatedScore);

  int GetId()const {
    return m_id;
  }

  const Hypothesis* GetPrevHypo() const;

  /** length of the partial translation (from the start of the sentence) */
  inline size_t GetSize() const {
    return m_currTargetWordsRange.GetEndPos() + 1;
  }

  std::string GetSourcePhraseStringRep(const std::vector<FactorType> factorsToPrint) const;
  std::string GetTargetPhraseStringRep(const std::vector<FactorType> factorsToPrint) const;
  std::string GetSourcePhraseStringRep() const;
  std::string GetTargetPhraseStringRep() const;

  /** curr - pos is relative from CURRENT hypothesis's starting index
   * (ie, start of sentence would be some negative number, which is
   * not allowed- USE WITH CAUTION) */
  inline const Word &GetCurrWord(size_t pos) const {
    return GetCurrTargetPhrase().GetWord(pos);
  }
  inline const Factor *GetCurrFactor(size_t pos, FactorType factorType) const {
    return GetCurrTargetPhrase().GetFactor(pos, factorType);
  }
  /** recursive - pos is relative from start of sentence */
  inline const Word &GetWord(size_t pos) const {
    const Hypothesis *hypo = this;
    while (pos < hypo->GetCurrTargetWordsRange().GetStartPos()) {
      hypo = hypo->GetPrevHypo();
      UTIL_THROW_IF2(hypo == NULL, "Previous hypothesis should not be NULL");
    }
    return hypo->GetCurrWord(pos - hypo->GetCurrTargetWordsRange().GetStartPos());
  }
  inline const Factor* GetFactor(size_t pos, FactorType factorType) const {
    return GetWord(pos)[factorType];
  }

  /***
   * \return The bitmap of source words we cover
   */
  inline const Bitmap &GetWordsBitmap() const {
    return m_sourceCompleted;
  }

  inline bool IsSourceCompleted() const {
    return m_sourceCompleted.IsComplete();
  }

  void GetOutputPhrase(Phrase &out) const;

  void ToStream(std::ostream& out) const {
    Phrase ret;
    GetOutputPhrase(ret);
    out << ret;
  }

  void ToStringStream(std::stringstream& out) const {
    if (m_prevHypo != NULL) {
      m_prevHypo->ToStream(out);
    }
    out << (const Phrase&) GetCurrTargetPhrase();
  }

  std::string GetOutputString() const {
    std::stringstream out;
    ToStringStream(out);
    return out.str();
  }

  TO_STRING();

  inline void SetWinningHypo(const Hypothesis *hypo) {
    m_winningHypo = hypo;
  }
  inline const Hypothesis *GetWinningHypo() const {
    return m_winningHypo;
  }

  void AddArc(Hypothesis *loserHypo);
  void CleanupArcList(size_t nBestSize, bool distinctNBest);

  //! returns a list alternative previous hypotheses (or NULL if n-best support is disabled)
  inline const ArcList* GetArcList() const {
    return m_arcList;
  }
  const ScoreComponentCollection& GetScoreBreakdown() const {
    if (!m_scoreBreakdown) {
      m_scoreBreakdown.reset(new ScoreComponentCollection);
      m_scoreBreakdown->PlusEquals(m_currScoreBreakdown);
      if (m_prevHypo) {
        m_scoreBreakdown->PlusEquals(m_prevHypo->GetScoreBreakdown());
      }
    }
    return *(m_scoreBreakdown.get());
  }
  float GetFutureScore() const {
    return m_futureScore;
  }
  float GetScore() const {
    return m_futureScore-m_estimatedScore;
  }
  const FFState* GetFFState(int idx) const {
    return m_ffStates[idx];
  }
  void SetFFState(int idx, FFState* state) {
    m_ffStates[idx] = state;
  }

  std::vector<std::vector<unsigned int> > *GetLMStats() const {
    return NULL;
  }

  const TranslationOption &GetTranslationOption() const {
    return m_transOpt;
  }

  size_t OutputAlignment(std::ostream &out, bool recursive) const;

  void OutputInput(std::ostream& os) const;
  static void OutputInput(std::vector<const Phrase*>& map, const Hypothesis* hypo);

  // creates a map of TARGET positions which should be replaced by word using placeholder
  std::map<size_t, const Moses::Factor*> GetPlaceholders(const Moses::Hypothesis &hypo, Moses::FactorType placeholderFactor) const;

  // for unordered_set in stack
  size_t hash() const;
  bool operator==(const Hypothesis& other) const;

#ifdef HAVE_XMLRPC_C
  // these are implemented in moses/server/Hypothesis_4server.cpp !
  void OutputWordAlignment(std::vector<xmlrpc_c::value>& out) const;
  void OutputLocalWordAlignment(std::vector<xmlrpc_c::value>& dest) const;
#endif

  bool beats(Hypothesis const& b) const;


};

std::ostream& operator<<(std::ostream& out, const Hypothesis& hypothesis);

// sorting helper
struct CompareHypothesisTotalScore {
  bool operator()(const Hypothesis* a, const Hypothesis* b) const {
    return a->beats(*b);
  }
};

}
#endif
