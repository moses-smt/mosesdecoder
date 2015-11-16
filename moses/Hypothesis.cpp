// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
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

#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>

#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "Hypothesis.h"
#include "Util.h"
#include "SquareMatrix.h"
#include "StaticData.h"
#include "InputType.h"
#include "Manager.h"
#include "IOWrapper.h"
#include "moses/FF/FFState.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"

#include <boost/foreach.hpp>

using namespace std;

namespace Moses
{
Hypothesis::
Hypothesis(Manager& manager, InputType const& source, const TranslationOption &initialTransOpt, const Bitmap &bitmap, int id)
  : m_prevHypo(NULL)
  , m_sourceCompleted(bitmap)
  , m_sourceInput(source)
  , m_currSourceWordsRange(
    m_sourceCompleted.GetFirstGapPos()>0 ? 0 : NOT_FOUND,
    m_sourceCompleted.GetFirstGapPos()>0 ? m_sourceCompleted.GetFirstGapPos()-1 : NOT_FOUND)
  , m_currTargetWordsRange(NOT_FOUND, NOT_FOUND)
  , m_wordDeleted(false)
  , m_futureScore(0.0f)
  , m_estimatedScore(0.0f)
  , m_ffStates(StatefulFeatureFunction::GetStatefulFeatureFunctions().size())
  , m_arcList(NULL)
  , m_transOpt(initialTransOpt)
  , m_manager(manager)
  , m_id(id)
{
  // used for initial seeding of trans process
  // initialize scores
  //_hash_computed = false;
  //s_HypothesesCreated = 1;
  const vector<const StatefulFeatureFunction*>& ffs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i)
    m_ffStates[i] = ffs[i]->EmptyHypothesisState(source);
}

/***
 * continue prevHypo by appending the phrases in transOpt
 */
Hypothesis::
Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt, const Bitmap &bitmap, int id)
  : m_prevHypo(&prevHypo)
  , m_sourceCompleted(bitmap)
  , m_sourceInput(prevHypo.m_sourceInput)
  , m_currSourceWordsRange(transOpt.GetSourceWordsRange())
  , m_currTargetWordsRange(prevHypo.m_currTargetWordsRange.GetEndPos() + 1,
                           prevHypo.m_currTargetWordsRange.GetEndPos()
                           + transOpt.GetTargetPhrase().GetSize())
  , m_wordDeleted(false)
  , m_futureScore(0.0f)
  , m_estimatedScore(0.0f)
  , m_ffStates(prevHypo.m_ffStates.size())
  , m_arcList(NULL)
  , m_transOpt(transOpt)
  , m_manager(prevHypo.GetManager())
  , m_id(id)
{
  m_currScoreBreakdown.PlusEquals(transOpt.GetScoreBreakdown());
  m_wordDeleted = transOpt.IsDeletionOption();
}

Hypothesis::
~Hypothesis()
{
  for (unsigned i = 0; i < m_ffStates.size(); ++i)
    delete m_ffStates[i];

  if (m_arcList) {
    ArcList::iterator iter;
    for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter) {
      delete *iter;
    }
    m_arcList->clear();

    delete m_arcList;
    m_arcList = NULL;
  }
}

void
Hypothesis::
AddArc(Hypothesis *loserHypo)
{
  if (!m_arcList) {
    if (loserHypo->m_arcList) { // we don't have an arcList, but loser does
      this->m_arcList = loserHypo->m_arcList;  // take ownership, we'll delete
      loserHypo->m_arcList = 0;                // prevent a double deletion
    } else {
      this->m_arcList = new ArcList();
    }
  } else {
    if (loserHypo->m_arcList) {  // both have an arc list: merge. delete loser
      size_t my_size = m_arcList->size();
      size_t add_size = loserHypo->m_arcList->size();
      this->m_arcList->resize(my_size + add_size, 0);
      std::memcpy(&(*m_arcList)[0] + my_size, &(*loserHypo->m_arcList)[0], add_size * sizeof(Hypothesis *));
      delete loserHypo->m_arcList;
      loserHypo->m_arcList = 0;
    } else { // loserHypo doesn't have any arcs
      // DO NOTHING
    }
  }
  m_arcList->push_back(loserHypo);
}

/***
 * calculate the logarithm of our total translation score (sum up components)
 */
void
Hypothesis::
EvaluateWhenApplied(float estimatedScore)
{
  const StaticData &staticData = StaticData::Instance();

  // some stateless score producers cache their values in the translation
  // option: add these here
  // language model scores for n-grams completely contained within a target
  // phrase are also included here

  // compute values of stateless feature functions that were not
  // cached in the translation option
  const vector<const StatelessFeatureFunction*>& sfs =
    StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for (unsigned i = 0; i < sfs.size(); ++i) {
    const StatelessFeatureFunction &ff = *sfs[i];
    if(!staticData.IsFeatureFunctionIgnored(ff)) {
      ff.EvaluateWhenApplied(*this, &m_currScoreBreakdown);
    }
  }

  const vector<const StatefulFeatureFunction*>& ffs =
    StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i) {
    const StatefulFeatureFunction &ff = *ffs[i];
    if(!staticData.IsFeatureFunctionIgnored(ff)) {
      FFState const* s = m_prevHypo ? m_prevHypo->m_ffStates[i] : NULL;
      m_ffStates[i] = ff.EvaluateWhenApplied(*this, s, &m_currScoreBreakdown);
    }
  }

  // FUTURE COST
  m_estimatedScore = estimatedScore;

  // TOTAL
  m_futureScore = m_currScoreBreakdown.GetWeightedScore() + m_estimatedScore;
  if (m_prevHypo) m_futureScore += m_prevHypo->GetScore();
}

const Hypothesis* Hypothesis::GetPrevHypo()const
{
  return m_prevHypo;
}

/**
 * print hypothesis information for pharaoh-style logging
 */
void
Hypothesis::
PrintHypothesis() const
{
  if (!m_prevHypo) {
    TRACE_ERR(endl << "NULL hypo" << endl);
    return;
  }
  TRACE_ERR(endl << "creating hypothesis "<< m_id <<" from "<< m_prevHypo->m_id<<" ( ");
  int end = (int)(m_prevHypo->GetCurrTargetPhrase().GetSize()-1);
  int start = end-1;
  if ( start < 0 ) start = 0;
  if ( m_prevHypo->m_currTargetWordsRange.GetStartPos() == NOT_FOUND ) {
    TRACE_ERR( "<s> ");
  } else {
    TRACE_ERR( "... ");
  }
  if (end>=0) {
    Range range(start, end);
    TRACE_ERR( m_prevHypo->GetCurrTargetPhrase().GetSubString(range) << " ");
  }
  TRACE_ERR( ")"<<endl);
  TRACE_ERR( "\tbase score "<< (m_prevHypo->m_futureScore - m_prevHypo->m_estimatedScore) <<endl);
  TRACE_ERR( "\tcovering "<<m_currSourceWordsRange.GetStartPos()<<"-"<<m_currSourceWordsRange.GetEndPos()
             <<": " << m_transOpt.GetInputPath().GetPhrase() << endl);

  TRACE_ERR( "\ttranslated as: "<<(Phrase&) GetCurrTargetPhrase()<<endl); // <<" => translation cost "<<m_score[ScoreType::PhraseTrans];

  if (m_wordDeleted) TRACE_ERR( "\tword deleted"<<endl);
  //	TRACE_ERR( "\tdistance: "<<GetCurrSourceWordsRange().CalcDistortion(m_prevHypo->GetCurrSourceWordsRange())); // << " => distortion cost "<<(m_score[ScoreType::Distortion]*weightDistortion)<<endl;
  //	TRACE_ERR( "\tlanguage model cost "); // <<m_score[ScoreType::LanguageModelScore]<<endl;
  //	TRACE_ERR( "\tword penalty "); // <<(m_score[ScoreType::WordPenalty]*weightWordPenalty)<<endl;
  TRACE_ERR( "\tscore "<<m_futureScore - m_estimatedScore<<" + future cost "<<m_estimatedScore<<" = "<<m_futureScore<<endl);
  TRACE_ERR(  "\tunweighted feature scores: " << m_currScoreBreakdown << endl);
  //PrintLMScores();
}

void
Hypothesis::
CleanupArcList(size_t nBestSize, bool distinctNBest)
{
  // point this hypo's main hypo to itself
  SetWinningHypo(this);

  if (!m_arcList) return;

  /* keep only number of arcs we need to create all n-best paths.
   * However, may not be enough if only unique candidates are needed,
   * so we'll keep all of arc list if nedd distinct n-best list
   */

  if (!distinctNBest && m_arcList->size() > nBestSize * 5) {
    // prune arc list only if there too many arcs
    NTH_ELEMENT4(m_arcList->begin(), m_arcList->begin() + nBestSize - 1,
                 m_arcList->end(), CompareHypothesisTotalScore());

    // delete bad ones
    ArcList::iterator i = m_arcList->begin() + nBestSize;
    while (i != m_arcList->end()) delete *i++;
    m_arcList->erase(m_arcList->begin() + nBestSize, m_arcList->end());
  }

  // set all arc's main hypo variable to this hypo
  ArcList::iterator iter = m_arcList->begin();
  for (; iter != m_arcList->end() ; ++iter) {
    Hypothesis *arc = *iter;
    arc->SetWinningHypo(this);
  }
}

TargetPhrase const&
Hypothesis::
GetCurrTargetPhrase() const
{
  return m_transOpt.GetTargetPhrase();
}

void
Hypothesis::
GetOutputPhrase(Phrase &out) const
{
  if (m_prevHypo != NULL)
    m_prevHypo->GetOutputPhrase(out);
  out.Append(GetCurrTargetPhrase());
}

TO_STRING_BODY(Hypothesis)

// friend
ostream& operator<<(ostream& out, const Hypothesis& hypo)
{
  hypo.ToStream(out);
  // words bitmap
  out << "[" << hypo.m_sourceCompleted << "] ";

  // scores
  out << " [total=" << hypo.GetFutureScore() << "]";
  out << " " << hypo.GetScoreBreakdown();

  // alignment
  out << " " << hypo.GetCurrTargetPhrase().GetAlignNonTerm();

  return out;
}


std::string
Hypothesis::
GetSourcePhraseStringRep(const vector<FactorType> factorsToPrint) const
{
  return m_transOpt.GetInputPath().GetPhrase().GetStringRep(factorsToPrint);
}

std::string
Hypothesis::
GetTargetPhraseStringRep(const vector<FactorType> factorsToPrint) const
{
  return (m_prevHypo
          ? GetCurrTargetPhrase().GetStringRep(factorsToPrint)
          : "");
}

std::string
Hypothesis::
GetSourcePhraseStringRep() const
{
  vector<FactorType> allFactors(MAX_NUM_FACTORS);
  for(size_t i=0; i < MAX_NUM_FACTORS; i++)
    allFactors[i] = i;
  return GetSourcePhraseStringRep(allFactors);
}

std::string
Hypothesis::
GetTargetPhraseStringRep() const
{
  vector<FactorType> allFactors(MAX_NUM_FACTORS);
  for(size_t i=0; i < MAX_NUM_FACTORS; i++)
    allFactors[i] = i;
  return GetTargetPhraseStringRep(allFactors);
}

void
Hypothesis::
OutputAlignment(std::ostream &out, WordAlignmentSort sortOrder) const
{
  std::vector<const Hypothesis *> edges;
  const Hypothesis *currentHypo = this;
  while (currentHypo) {
    edges.push_back(currentHypo);
    currentHypo = currentHypo->GetPrevHypo();
  }

  OutputAlignment(out, edges, sortOrder);

}

void
Hypothesis::
OutputAlignment(ostream &out,
                vector<const Hypothesis *> const& edges,
                WordAlignmentSort waso)
{
  size_t targetOffset = 0;

  for (int currEdge = (int)edges.size() - 1 ; currEdge >= 0 ; currEdge--) {
    const Hypothesis &edge = *edges[currEdge];
    const TargetPhrase &tp = edge.GetCurrTargetPhrase();
    size_t sourceOffset = edge.GetCurrSourceWordsRange().GetStartPos();

    OutputAlignment(out, tp.GetAlignTerm(), sourceOffset, targetOffset, waso);

    targetOffset += tp.GetSize();
  }
  // Used by --print-alignment-info, so no endl
}

void
Hypothesis::
OutputAlignment(ostream &out, const AlignmentInfo &ai,
                size_t sourceOffset, size_t targetOffset,
                WordAlignmentSort waso)
{
  typedef std::vector< const std::pair<size_t,size_t>* > AlignVec;
  AlignVec alignments = ai.GetSortedAlignments(waso);

  AlignVec::const_iterator it;
  for (it = alignments.begin(); it != alignments.end(); ++it) {
    const std::pair<size_t,size_t> &alignment = **it;
    out << alignment.first  + sourceOffset << "-"
        << alignment.second + targetOffset << " ";
  }

}

void
Hypothesis::
OutputInput(std::vector<const Phrase*>& map, const Hypothesis* hypo)
{
  if (!hypo->GetPrevHypo()) return;
  OutputInput(map, hypo->GetPrevHypo());
  map[hypo->GetCurrSourceWordsRange().GetStartPos()]
  = &hypo->GetTranslationOption().GetInputPath().GetPhrase();
}

void
Hypothesis::
OutputInput(std::ostream& os) const
{
  size_t len = this->GetInput().GetSize();
  std::vector<const Phrase*> inp_phrases(len, 0);
  OutputInput(inp_phrases, this);
  for (size_t i=0; i<len; ++i)
    if (inp_phrases[i]) os << *inp_phrases[i];
}

void
Hypothesis::
OutputBestSurface(std::ostream &out, const std::vector<FactorType> &outputFactorOrder,
                  const ReportingOptions &options) const
{
  if (m_prevHypo) {
    // recursively retrace this best path through the lattice, starting from the end of the hypothesis sentence
    m_prevHypo->OutputBestSurface(out, outputFactorOrder, options);
  }
  OutputSurface(out, *this, outputFactorOrder, options);
}

//////////////////////////////////////////////////////////////////////////
/***
 * print surface factor only for the given phrase
 */
void
Hypothesis::
OutputSurface(std::ostream &out, const Hypothesis &edge,
              const std::vector<FactorType> &outputFactorOrder,
              const ReportingOptions &options) const
{
  UTIL_THROW_IF2(outputFactorOrder.size() == 0,
                 "Must specific at least 1 output factor");
  const TargetPhrase& phrase = edge.GetCurrTargetPhrase();
  // TODO: slay the rest of StaticData here and move stuff into ReportingOptions
  bool markUnknown = GetManager().options().unk.mark;
  bool featureLabels = StaticData::Instance().options().nbest.include_feature_labels;
  if (options.ReportAllFactors == true) {
    out << phrase;
  } else {
    FactorType placeholderFactor
    = StaticData::Instance().options().input.placeholder_factor;

    std::map<size_t, const Factor*> placeholders;
    if (placeholderFactor != NOT_FOUND) {
      // creates map of target position -> factor for placeholders
      placeholders = GetPlaceholders(edge, placeholderFactor);
    }

    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);

      if (placeholders.size()) {
        // do placeholders
        std::map<size_t, const Factor*>::const_iterator iter = placeholders.find(pos);
        if (iter != placeholders.end()) {
          factor = iter->second;
        }
      }

      UTIL_THROW_IF2(factor == NULL,
                     "No factor 0 at position " << pos);

      //preface surface form with UNK if marking unknowns
      const Word &word = phrase.GetWord(pos);
      if(markUnknown && word.IsOOV()) {
        out << GetManager().options().unk.prefix << *factor
            << GetManager().options().unk.suffix;
      } else {
        out << *factor;
      }

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
        UTIL_THROW_IF2(factor == NULL,
                       "No factor " << i << " at position " << pos);

        out << "|" << *factor;
      }
      out << " ";
    }
  }

  // trace ("report segmentation") option "-t" / "-tt"
  if (options.ReportSegmentation > 0 && phrase.GetSize() > 0) {
    const Range &sourceRange = edge.GetCurrSourceWordsRange();
    const int sourceStart = sourceRange.GetStartPos();
    const int sourceEnd = sourceRange.GetEndPos();
    out << "|" << sourceStart << "-" << sourceEnd;    // enriched "-tt"
    if (options.ReportSegmentation == 2) {
      out << ",wa=";
      const AlignmentInfo &ai = edge.GetCurrTargetPhrase().GetAlignTerm();
      Hypothesis::OutputAlignment(out, ai, 0, 0, options.WA_SortOrder);
      out << ",total=";
      out << edge.GetScore() - edge.GetPrevHypo()->GetScore();
      out << ",";
      ScoreComponentCollection scoreBreakdown(edge.GetScoreBreakdown());
      scoreBreakdown.MinusEquals(edge.GetPrevHypo()->GetScoreBreakdown());
      scoreBreakdown.OutputAllFeatureScores(out, featureLabels);
    }
    out << "| ";
  }
}

std::map<size_t, const Factor*>
Hypothesis::
GetPlaceholders(const Hypothesis &hypo, FactorType placeholderFactor) const
{
  const InputPath &inputPath = hypo.GetTranslationOption().GetInputPath();
  const Phrase &inputPhrase = inputPath.GetPhrase();

  std::map<size_t, const Factor*> ret;

  for (size_t sourcePos = 0; sourcePos < inputPhrase.GetSize(); ++sourcePos) {
    const Factor *factor = inputPhrase.GetFactor(sourcePos, placeholderFactor);
    if (factor) {
      std::set<size_t> targetPos = hypo.GetTranslationOption().GetTargetPhrase().GetAlignTerm().GetAlignmentsForSource(sourcePos);
      UTIL_THROW_IF2(targetPos.size() != 1,
                     "Placeholder should be aligned to 1, and only 1, word");
      ret[*targetPos.begin()] = factor;
    }
  }

  return ret;
}

size_t Hypothesis::hash() const
{
  size_t seed;

  // coverage NOTE from Hieu - we could make bitmap comparison here
  // and in operator== compare the pointers since the bitmaps come
  // from a factory.  Same coverage is guaranteed to have the same
  // bitmap. However, this make the decoding algorithm
  // non-deterministic as the order of hypo extension can be
  // different. This causes several regression tests to break. Since
  // the speedup is minimal, I'm gonna leave it comparing the actual
  // bitmaps
  seed = m_sourceCompleted.hash();

  // states
  for (size_t i = 0; i < m_ffStates.size(); ++i) {
    const FFState *state = m_ffStates[i];
    size_t hash = state->hash();
    boost::hash_combine(seed, hash);
  }
  return seed;
}

bool Hypothesis::operator==(const Hypothesis& other) const
{
  // coverage
  if (&m_sourceCompleted != &other.m_sourceCompleted) {
    return false;
  }

  // states
  for (size_t i = 0; i < m_ffStates.size(); ++i) {
    const FFState &thisState = *m_ffStates[i];
    const FFState &otherState = *other.m_ffStates[i];
    if (thisState != otherState) {
      return false;
    }
  }
  return true;
}

bool
Hypothesis::
beats(Hypothesis const& b) const
{
  if (m_futureScore != b.m_futureScore)
    return m_futureScore > b.m_futureScore;
  else if (m_estimatedScore != b.m_estimatedScore)
    return m_estimatedScore > b.m_estimatedScore;
  else if (m_prevHypo)
    return b.m_prevHypo ? m_prevHypo->beats(*b.m_prevHypo) : true;
  else return false;
  // TO DO: add more tie breaking here
  // results. We should compare other property of the hypos here.
  // On the other hand, how likely is this going to happen?
}

}

