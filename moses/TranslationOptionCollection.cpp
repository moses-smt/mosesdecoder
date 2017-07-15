// $Id$
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

#include <typeinfo>
#include <algorithm>
#include <typeinfo>
#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LM/Base.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "Util.h"
#include "StaticData.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "DecodeGraph.h"
#include "InputPath.h"
#include "moses/FF/UnknownWordPenaltyProducer.h"
#include "moses/FF/LexicalReordering/LexicalReordering.h"
#include "moses/FF/InputFeature.h"
#include "TranslationTask.h"
#include "util/exception.hh"

#include <boost/foreach.hpp>
using namespace std;

namespace Moses
{

/** constructor; since translation options are indexed by coverage span, the
 * corresponding data structure is initialized here This fn should be
 * called by inherited classe */
TranslationOptionCollection::
TranslationOptionCollection(ttasksptr const& ttask,
                            InputType const& src)
  : m_ttask(ttask)
  , m_source(src)
  , m_estimatedScores(src.GetSize())
  , m_maxNoTransOptPerCoverage(ttask->options()->search.max_trans_opt_per_cov)
  , m_translationOptionThreshold(ttask->options()->search.trans_opt_threshold)
  , m_max_phrase_length(ttask->options()->search.max_phrase_length)
  , max_partial_trans_opt(ttask->options()->search.max_partial_trans_opt)
{
  // create 2-d vector
  size_t size = src.GetSize();
  for (size_t sPos = 0 ; sPos < size ; ++sPos) {
    m_collection.push_back( vector< TranslationOptionList >() );

    size_t maxSize = size - sPos;
    maxSize = std::min(maxSize, m_max_phrase_length);

    for (size_t ePos = 0 ; ePos < maxSize ; ++ePos) {
      m_collection[sPos].push_back( TranslationOptionList() );
    }
  }
}

/** destructor, clears out data structures */
TranslationOptionCollection::
~TranslationOptionCollection()
{
  RemoveAllInColl(m_inputPathQueue);
}

void
TranslationOptionCollection::
Prune()
{
  static float no_th = -std::numeric_limits<float>::infinity();

  if (m_maxNoTransOptPerCoverage == 0 && m_translationOptionThreshold == no_th)
    return;

  // bookkeeping for how many options used, pruned
  size_t total = 0;
  size_t totalPruned = 0;

  // loop through all spans
  size_t size = m_source.GetSize();
  for (size_t sPos = 0 ; sPos < size; ++sPos) {
    BOOST_FOREACH(TranslationOptionList& fullList, m_collection[sPos]) {
      total       += fullList.size();
      totalPruned += fullList.SelectNBest(m_maxNoTransOptPerCoverage);
      totalPruned += fullList.PruneByThreshold(m_translationOptionThreshold);
    }
  }

  VERBOSE(2,"       Total translation options: " << total << std::endl
          << "Total translation options pruned: " << totalPruned << std::endl);
}

/** Force a creation of a translation option where there are none for a
 * particular source position.  ie. where a source word has not been
 * translated, create a translation option by
 * 1. not observing the table limits on phrase/generation tables
 * 2. using the handler ProcessUnknownWord()
 * Call this function once translation option collection has been filled with
 * translation options
 *
 * This function calls for unknown words is complicated by the fact it must
 * handle different input types.  The call stack is
 * Base::ProcessUnknownWord()
 * Inherited::ProcessUnknownWord(position)
 * Base::ProcessOneUnknownWord()
 *
 */

void
TranslationOptionCollection::
ProcessUnknownWord()
{
  const vector<DecodeGraph*>& decodeGraphList
  = StaticData::Instance().GetDecodeGraphs();
  size_t size = m_source.GetSize();
  // try to translation for coverage with no trans by expanding table limit
  for (size_t graphInd = 0 ; graphInd < decodeGraphList.size() ; graphInd++) {
    const DecodeGraph &decodeGraph = *decodeGraphList[graphInd];
    for (size_t pos = 0 ; pos < size ; ++pos) {
      TranslationOptionList* fullList = GetTranslationOptionList(pos, pos);
      // size_t numTransOpt = fullList.size();
      if (!fullList || fullList->size() == 0) {
        CreateTranslationOptionsForRange(decodeGraph, pos, pos, false, graphInd);
      }
    }
  }

  // bool alwaysCreateDirectTranslationOption
  // = StaticData::Instance().IsAlwaysCreateDirectTranslationOption();
  bool always = m_ttask.lock()->options()->unk.always_create_direct_transopt;

  // create unknown words for 1 word coverage where we don't have any trans options
  for (size_t pos = 0 ; pos < size ; ++pos) {
    TranslationOptionList* fullList = GetTranslationOptionList(pos, pos);
    if (!fullList || fullList->size() == 0 || always)
      ProcessUnknownWord(pos);
  }
}

/** special handling of ONE unknown words. Either add temporarily add word to
 * translation table, or drop the translation.  This function should be
 * called by the ProcessOneUnknownWord() in the inherited class At the
 * moment, this unknown word handler is a bit of a hack, if copies over
 * each factor from source to target word, or uses the 'UNK' factor.
 * Ideally, this function should be in a class which can be expanded
 * upon, for example, to create a morphologically aware handler.
 *
 * \param sourceWord the unknown word
 * \param sourcePos
 * \param length length covered by this word (may be > 1 for lattice input)
 * \param inputScores a set of scores associated with unknown word (input scores from latties/CNs)
 */
void
TranslationOptionCollection::
ProcessOneUnknownWord(const InputPath &inputPath, size_t sourcePos,
                      size_t length, const ScorePair *inputScores)
{
  const UnknownWordPenaltyProducer&
  unknownWordPenaltyProducer = UnknownWordPenaltyProducer::Instance();
  float unknownScore = FloorScore(TransformScore(0));
  const Word &sourceWord = inputPath.GetPhrase().GetWord(0);

  // hack. Once the OOV FF is a phrase table, get rid of this
  PhraseDictionary *firstPt = NULL;
  if (PhraseDictionary::GetColl().size() == 0) {
    firstPt = PhraseDictionary::GetColl()[0];
  }

  // unknown word, add as trans opt
  FactorCollection &factorCollection = FactorCollection::Instance();

  size_t isDigit = 0;

  const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
  const StringPiece s = f->GetString();
  bool isEpsilon = (s=="" || s==EPSILON);
  bool dropUnk = GetTranslationTask()->options()->unk.drop;
  if (dropUnk) {
    isDigit = s.find_first_of("0123456789");
    if (isDigit == string::npos)
      isDigit = 0;
    else
      isDigit = 1;
    // modify the starting bitmap
  }

  TargetPhrase targetPhrase(firstPt);

  if (!(dropUnk || isEpsilon) || isDigit) {
    // add to dictionary

    Word &targetWord = targetPhrase.AddWord();
    targetWord.SetIsOOV(true);

    for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++) {
      FactorType factorType = static_cast<FactorType>(currFactor);

      const Factor *sourceFactor = sourceWord[currFactor];
      if (sourceFactor == NULL)
        targetWord[factorType] = factorCollection.AddFactor(UNKNOWN_FACTOR);
      else
        targetWord[factorType] = factorCollection.AddFactor(sourceFactor->GetString());
    }
    //create a one-to-one alignment between UNKNOWN_FACTOR and its verbatim translation

    targetPhrase.SetAlignmentInfo("0-0");

  }

  targetPhrase.GetScoreBreakdown().Assign(&unknownWordPenaltyProducer, unknownScore);

  // source phrase
  const Phrase &sourcePhrase = inputPath.GetPhrase();
  m_unksrcs.push_back(&sourcePhrase);
  Range range(sourcePos, sourcePos + length - 1);

  targetPhrase.EvaluateInIsolation(sourcePhrase);

  TranslationOption *transOpt = new TranslationOption(range, targetPhrase);
  transOpt->SetInputPath(inputPath);
  Add(transOpt);


}

/** compute future score matrix in a dynamic programming fashion.
 * This matrix used in search.
 * Call this function once translation option collection has been filled with translation options
 */
void
TranslationOptionCollection::
CalcEstimatedScore()
{
  // setup the matrix (ignore lower triangle, set upper triangle to -inf
  m_estimatedScores.InitTriangle(-numeric_limits<float>::infinity());

  // walk all the translation options and record the cheapest option for each span
  size_t size = m_source.GetSize(); // the width of the matrix
  for (size_t sPos = 0 ; sPos < size ; ++sPos) {
    size_t ePos = sPos;
    BOOST_FOREACH(TranslationOptionList& tol, m_collection[sPos]) {
      TranslationOptionList::const_iterator toi;
      for(toi = tol.begin() ; toi != tol.end() ; ++toi) {
        const TranslationOption& to = **toi;
        float score = to.GetFutureScore();
        if (score > m_estimatedScores.GetScore(sPos, ePos))
          m_estimatedScores.SetScore(sPos, ePos, score);
      }
      ++ePos;
    }
  }

  // now fill all the cells in the strictly upper triangle
  //   there is no way to modify the diagonal now, in the case
  //   where no translation option covers a single-word span,
  //   we leave the +inf in the matrix
  // like in chart parsing we want each cell to contain the highest score
  // of the full-span trOpt or the sum of scores of joining two smaller spans

  for(size_t colstart = 1; colstart < size ; colstart++) {
    for(size_t diagshift = 0; diagshift < size-colstart ; diagshift++) {
      size_t sPos = diagshift;
      size_t ePos = colstart+diagshift;
      for(size_t joinAt = sPos; joinAt < ePos ; joinAt++)  {
        float joinedScore = m_estimatedScores.GetScore(sPos, joinAt)
                            + m_estimatedScores.GetScore(joinAt+1, ePos);
        // uncomment to see the cell filling scheme
        // TRACE_ERR("[" << sPos << "," << ePos << "] <-? ["
        // 	  << sPos << "," << joinAt << "]+["
        // 	  << joinAt+1 << "," << ePos << "] (colstart: "
        // 	  << colstart << ", diagshift: " << diagshift << ")"
        // 	  << endl);

        if (joinedScore > m_estimatedScores.GetScore(sPos, ePos))
          m_estimatedScores.SetScore(sPos, ePos, joinedScore);
      }
    }
  }

  IFVERBOSE(3) {
    int total = 0;
    for(size_t row = 0; row < size; row++) {
      size_t col = row;
      BOOST_FOREACH(TranslationOptionList& tol, m_collection[row]) {
        // size_t maxSize = size - row;
        // size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
        // maxSize = std::min(maxSize, maxSizePhrase);

        // for(size_t col=row; col<row+maxSize; col++) {
        int count = tol.size();
        TRACE_ERR( "translation options spanning from  "
                   << row <<" to "<< col <<" is "
                   << count <<endl);
        total += count;
        ++col;
      }
    }
    TRACE_ERR( "translation options generated in total: "<< total << endl);

    for(size_t row=0; row<size; row++)
      for(size_t col=row; col<size; col++)
        TRACE_ERR( "future cost from "<< row <<" to "<< col <<" is "
                   << m_estimatedScores.GetScore(row, col) <<endl);
  }
}



/** Create all possible translations from the phrase tables
 * for a particular input sentence. This implies applying all
 * translation and generation steps. Also computes future cost matrix.
 */
void
TranslationOptionCollection::
CreateTranslationOptions()
{
  // loop over all substrings of the source sentence, look them up
  // in the phraseDictionary (which is the- possibly filtered-- phrase
  // table loaded on initialization), generate TranslationOption objects
  // for all phrases

  // there may be multiple decoding graphs (factorizations of decoding)
  const vector <DecodeGraph*> &decodeGraphList
  = StaticData::Instance().GetDecodeGraphs();

  // length of the sentence
  const size_t size = m_source.GetSize();

  // loop over all decoding graphs, each generates translation options
  for (size_t gidx = 0 ; gidx < decodeGraphList.size() ; gidx++) {
    if (decodeGraphList.size() > 1)
      VERBOSE(3,"Creating translation options from decoding graph " << gidx << endl);

    const DecodeGraph& dg = *decodeGraphList[gidx];
    size_t backoff = dg.GetBackoff();
    // iterate over spans
    for (size_t sPos = 0 ; sPos < size; sPos++) {
      size_t maxSize = size - sPos; // don't go over end of sentence
      // size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = std::min(maxSize, m_max_phrase_length);

      for (size_t ePos = sPos ; ePos < sPos + maxSize ; ePos++) {
        if (gidx && backoff &&
            (ePos-sPos+1 <= backoff || // size exceeds backoff limit (HUH? UG) or ...
             m_collection[sPos][ePos-sPos].size() > 0)) {
          VERBOSE(3,"No backoff to graph " << gidx << " for span [" << sPos << ";" << ePos << "]" << endl);
          continue;
        }
        CreateTranslationOptionsForRange(dg, sPos, ePos, true, gidx);
      }
    }
  }
  ProcessUnknownWord();
  EvaluateWithSourceContext();
  VERBOSE(3,"Translation Option Collection\n " << *this << endl);
  Prune();
  Sort();
  CalcEstimatedScore(); // future score matrix
  CacheLexReordering(); // Cached lex reodering costs
}


bool
TranslationOptionCollection::
CreateTranslationOptionsForRange
(const DecodeGraph& dgraph, size_t sPos, size_t ePos,
 bool adhereTableLimit, size_t gidx, InputPath &inputPath)
{
  typedef DecodeStepTranslation Tstep;
  typedef DecodeStepGeneration Gstep;
  XmlInputType xml_policy = m_ttask.lock()->options()->input.xml_policy;
  if ((xml_policy != XmlExclusive)
      || !HasXmlOptionsOverlappingRange(sPos,ePos)) {

    // partial trans opt stored in here
    PartialTranslOptColl* oldPtoc = new PartialTranslOptColl(max_partial_trans_opt);
    size_t totalEarlyPruned = 0;

    // initial translation step
    list <const DecodeStep* >::const_iterator d = dgraph.begin();
    const DecodeStep &dstep = **d;

    const PhraseDictionary &pdict = *dstep.GetPhraseDictionaryFeature();
    TargetPhraseCollection::shared_ptr targetPhrases = inputPath.GetTargetPhrases(pdict);

    static_cast<const Tstep&>(dstep).ProcessInitialTranslation
    (m_source, *oldPtoc, sPos, ePos, adhereTableLimit, inputPath, targetPhrases);

    SetInputScore(inputPath, *oldPtoc);

    // do rest of decode steps
    int indexStep = 0;

    for (++d ; d != dgraph.end() ; ++d) {
      const DecodeStep *dstep = *d;
      PartialTranslOptColl* newPtoc = new PartialTranslOptColl(m_max_phrase_length);

      // go thru each intermediate trans opt just created
      const vector<TranslationOption*>& partTransOptList = oldPtoc->GetList();
      vector<TranslationOption*>::const_iterator pto;
      for (pto = partTransOptList.begin() ; pto != partTransOptList.end() ; ++pto) {
        TranslationOption &inputPartialTranslOpt = **pto;
        if (const Tstep *tstep = dynamic_cast<const Tstep*>(dstep)) {
          const PhraseDictionary &pdict = *tstep->GetPhraseDictionaryFeature();
          TargetPhraseCollection::shared_ptr targetPhrases = inputPath.GetTargetPhrases(pdict);
          tstep->Process(inputPartialTranslOpt, *dstep, *newPtoc,
                         this, adhereTableLimit, targetPhrases);
        } else {
          const Gstep *genStep = dynamic_cast<const Gstep*>(dstep);
          UTIL_THROW_IF2(!genStep, "Decode steps must be either "
                         << "Translation or Generation Steps!");
          genStep->Process(inputPartialTranslOpt, *dstep, *newPtoc,
                           this, adhereTableLimit);
        }
      }

      // last but 1 partial trans not required anymore
      totalEarlyPruned += newPtoc->GetPrunedCount();
      delete oldPtoc;
      oldPtoc = newPtoc;

      indexStep++;
    } // for (++d

    // add to fully formed translation option list
    PartialTranslOptColl &lastPartialTranslOptColl	= *oldPtoc;
    const vector<TranslationOption*>& partTransOptList = lastPartialTranslOptColl.GetList();
    vector<TranslationOption*>::const_iterator c;
    for (c = partTransOptList.begin() ; c != partTransOptList.end() ; ++c) {
      TranslationOption *transOpt = *c;
      if (xml_policy != XmlConstraint ||
          !ViolatesXmlOptionsConstraint(sPos,ePos,transOpt)) {
        Add(transOpt);
      }
    }
    lastPartialTranslOptColl.DetachAll();
    totalEarlyPruned += oldPtoc->GetPrunedCount();
    delete oldPtoc;
    // TRACE_ERR( "Early translation options pruned: " << totalEarlyPruned << endl);
  } // if ((xml_policy != XmlExclusive) || !HasXmlOptionsOverlappingRange(sPos,ePos))

  if (gidx == 0 && xml_policy != XmlPassThrough
      && HasXmlOptionsOverlappingRange(sPos,ePos)) {
    CreateXmlOptionsForRange(sPos, ePos);
  }

  return true;
}

void
TranslationOptionCollection::
SetInputScore(const InputPath &inputPath, PartialTranslOptColl &oldPtoc)
{
  const ScorePair* inputScore = inputPath.GetInputScore();
  if (inputScore == NULL) return;

  const InputFeature *inputFeature = InputFeature::InstancePtr();

  const std::vector<TranslationOption*> &transOpts = oldPtoc.GetList();
  for (size_t i = 0; i < transOpts.size(); ++i) {
    TranslationOption &transOpt = *transOpts[i];

    ScoreComponentCollection &scores = transOpt.GetScoreBreakdown();
    scores.PlusEquals(inputFeature, *inputScore);

  }
}

void
TranslationOptionCollection::
EvaluateWithSourceContext()
{
  const size_t size = m_source.GetSize();
  for (size_t sPos = 0 ; sPos < size ; ++sPos) {
    BOOST_FOREACH(TranslationOptionList& tol, m_collection[sPos]) {
      typedef TranslationOptionList::const_iterator to_iter;
      for(to_iter i = tol.begin() ; i != tol.end() ; ++i)
        (*i)->EvaluateWithSourceContext(m_source);
      EvaluateTranslationOptionListWithSourceContext(tol);
    }
  }
}

void TranslationOptionCollection::EvaluateTranslationOptionListWithSourceContext(
  TranslationOptionList &translationOptionList)
{

  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  const StaticData &staticData = StaticData::Instance();
  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    if (! staticData.IsFeatureFunctionIgnored(ff)) {
      ff.EvaluateTranslationOptionListWithSourceContext(m_source, translationOptionList);
    }
  }

}

void
TranslationOptionCollection::
Sort()
{
  static TranslationOption::Better cmp;
  size_t size = m_source.GetSize();
  for (size_t sPos = 0 ; sPos < size; ++sPos) {
    BOOST_FOREACH(TranslationOptionList& tol, m_collection.at(sPos)) {
      // cerr << sPos << ": " << tol.size() << " "
      // << __FILE__ << ":" << __LINE__ << endl;
      // size_t nulls=0;
      // BOOST_FOREACH(TranslationOption const* t, tol)
      //   if (t == NULL) ++nulls;
      // cerr << nulls << " null pointers ;"
      // << __FILE__ << ":" << __LINE__ << endl;
      std::sort(tol.begin(), tol.end(), cmp);
    }
  }
}

/** Check if this range overlaps with any XML options. This doesn't need to be an exact match, only an overlap.
 * by default, we don't support XML options. subclasses need to override this function.
 * called by CreateTranslationOptionsForRange()
 * \param sPos first position in input sentence
 * \param lastPos last position in input sentence
 */
bool
TranslationOptionCollection::
HasXmlOptionsOverlappingRange(size_t, size_t) const
{
  return false;
}

/** Check if an option conflicts with any constraint XML options. Okay, if XML option is substring in source and target.
 * by default, we don't support XML options. subclasses need to override this function.
 * called by CreateTranslationOptionsForRange()
 * \param sPos first position in input sentence
 * \param lastPos last position in input sentence
 */
bool
TranslationOptionCollection::
ViolatesXmlOptionsConstraint(size_t, size_t, TranslationOption*) const
{
  return false;
}

/** Populates the current Collection with XML options exactly covering the range specified. Default implementation does nothing.
 * called by CreateTranslationOptionsForRange()
 * \param sPos first position in input sentence
 * \param lastPos last position in input sentence
 */
void
TranslationOptionCollection::
CreateXmlOptionsForRange(size_t, size_t)
{ }


/** Add translation option to the list
 * \param translationOption translation option to be added */
void
TranslationOptionCollection::
Add(TranslationOption *translationOption)
{
  const Range &coverage = translationOption->GetSourceWordsRange();
  size_t const s = coverage.GetStartPos();
  size_t const e = coverage.GetEndPos();
  size_t const i = e - s;

  UTIL_THROW_IF2(e >= m_source.GetSize(),
                 "Coverage exceeds input size:" << coverage << "\n"
                 << "translationOption=" << *translationOption);

  vector<TranslationOptionList>& v = m_collection[s];
  while (i >= v.size()) v.push_back(TranslationOptionList());
  v[i].Add(translationOption);
}

TO_STRING_BODY(TranslationOptionCollection);

std::ostream&
operator<<(std::ostream& out, const TranslationOptionCollection& coll)
{
  size_t stop = coll.m_source.GetSize();
  TranslationOptionList const* tol;
  for (size_t sPos = 0 ; sPos < stop ; ++sPos) {
    for (size_t ePos = sPos;
         (tol = coll.GetTranslationOptionList(sPos, ePos)) != NULL;
         ++ePos) {
      BOOST_FOREACH(TranslationOption const* to, *tol)
      out << *to << std::endl;
    }
  }
  return out;
}

void
TranslationOptionCollection::
CacheLexReordering()
{
  size_t const stop = m_source.GetSize();
  typedef StatefulFeatureFunction sfFF;
  BOOST_FOREACH(sfFF const* ff, sfFF::GetStatefulFeatureFunctions()) {
    if (typeid(*ff) != typeid(LexicalReordering)) continue;
    LexicalReordering const& lr = static_cast<const LexicalReordering&>(*ff);
    for (size_t s = 0 ; s < stop ; s++)
      BOOST_FOREACH(TranslationOptionList& tol, m_collection[s])
      lr.SetCache(tol);
  }
}

//! list of trans opt for a particular span
TranslationOptionList*
TranslationOptionCollection::
GetTranslationOptionList(size_t const sPos, size_t const ePos)
{
  UTIL_THROW_IF2(sPos >= m_collection.size(), "Out of bound access.");
  vector<TranslationOptionList>& tol = m_collection[sPos];
  size_t idx = ePos - sPos;
  return idx < tol.size() ? &tol[idx] : NULL;
}

TranslationOptionList const*
TranslationOptionCollection::
GetTranslationOptionList(size_t sPos, size_t ePos) const
{
  UTIL_THROW_IF2(sPos >= m_collection.size(), "Out of bound access.");
  vector<TranslationOptionList> const& tol = m_collection[sPos];
  size_t idx = ePos - sPos;
  return idx < tol.size() ? &tol[idx] : NULL;
}

void
TranslationOptionCollection::
GetTargetPhraseCollectionBatch()
{
  typedef DecodeStepTranslation Tstep;
  const vector <DecodeGraph*> &dgl = StaticData::Instance().GetDecodeGraphs();
  BOOST_FOREACH(DecodeGraph const* dgraph, dgl) {
    typedef list <const DecodeStep* >::const_iterator dsiter;
    for (dsiter i = dgraph->begin(); i != dgraph->end() ; ++i) {
      const Tstep* tstep = dynamic_cast<const Tstep *>(*i);
      if (tstep) {
        const PhraseDictionary &pdict = *tstep->GetPhraseDictionaryFeature();
        pdict.GetTargetPhraseCollectionBatch(m_ttask.lock(), m_inputPathQueue);
      }
    }
  }
}

} // namespace

