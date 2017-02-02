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

#include "TrellisPath.h"
#include "TrellisPathList.h"
#include "TrellisPathCollection.h"
#include "StaticData.h"
#include "Manager.h"
using namespace std;

namespace Moses
{
TrellisPath::TrellisPath(const Hypothesis *hypo)
  : m_prevEdgeChanged(NOT_FOUND)
{
  m_totalScore = hypo->GetFutureScore();

  // enumerate path using prevHypo
  while (hypo != NULL) {
    m_path.push_back(hypo);
    hypo = hypo->GetPrevHypo();
  }
}

void TrellisPath::InitTotalScore()
{
  m_totalScore = m_path[0]->GetWinningHypo()->GetFutureScore();

  //calc score
  size_t sizePath = m_path.size();
  for (size_t pos = 0 ; pos < sizePath ; pos++) {
    const Hypothesis *hypo = m_path[pos];
    const Hypothesis *winningHypo = hypo->GetWinningHypo();
    if (hypo != winningHypo) {
      m_totalScore += hypo->GetFutureScore() - winningHypo->GetFutureScore();
    }
  }
}

TrellisPath::TrellisPath(const TrellisPath &copy, size_t edgeIndex, const Hypothesis *arc)
  :m_prevEdgeChanged(edgeIndex)
{
  m_path.reserve(copy.m_path.size());
  for (size_t currEdge = 0 ; currEdge < edgeIndex ; currEdge++) {
    // copy path from parent
    m_path.push_back(copy.m_path[currEdge]);
  }

  // 1 deviation
  m_path.push_back(arc);

  // rest of path comes from following best path backwards
  const Hypothesis *prevHypo = arc->GetPrevHypo();
  while (prevHypo != NULL) {
    m_path.push_back(prevHypo);
    prevHypo = prevHypo->GetPrevHypo();
  }

  InitTotalScore();
}

TrellisPath::TrellisPath(const vector<const Hypothesis*> edges)
  :m_prevEdgeChanged(NOT_FOUND)
{
  m_path.resize(edges.size());
  copy(edges.rbegin(),edges.rend(),m_path.begin());
  InitTotalScore();
}


void TrellisPath::CreateDeviantPaths(TrellisPathCollection &pathColl) const
{
  const size_t sizePath = m_path.size();

  if (m_prevEdgeChanged == NOT_FOUND) {
    // initial enumration from a pure hypo
    for (size_t currEdge = 0 ; currEdge < sizePath ; currEdge++) {
      const Hypothesis	*hypo		= static_cast<const Hypothesis*>(m_path[currEdge]);
      const ArcList *pAL = hypo->GetArcList();
      if (!pAL) continue;
      const ArcList &arcList = *pAL;

      // every possible Arc to replace this edge
      ArcList::const_iterator iterArc;
      for (iterArc = arcList.begin() ; iterArc != arcList.end() ; ++iterArc) {
        const Hypothesis *arc = *iterArc;
        TrellisPath *deviantPath = new TrellisPath(*this, currEdge, arc);
        pathColl.Add(deviantPath);
      }
    }
  } else {
    // wiggle 1 of the edges only
    for (size_t currEdge = m_prevEdgeChanged + 1 ; currEdge < sizePath ; currEdge++) {
      const ArcList *pAL = m_path[currEdge]->GetArcList();
      if (!pAL) continue;
      const ArcList &arcList = *pAL;
      ArcList::const_iterator iterArc;

      for (iterArc = arcList.begin() ; iterArc != arcList.end() ; ++iterArc) {
        // copy this Path & change 1 edge
        const Hypothesis *arcReplace = *iterArc;

        TrellisPath *deviantPath = new TrellisPath(*this, currEdge, arcReplace);
        pathColl.Add(deviantPath);
      } // for (iterArc...
    } // for (currEdge = 0 ...
  }
}

void TrellisPath::CreateDeviantPaths(TrellisPathList &pathColl) const
{
  const size_t sizePath = m_path.size();

  if (m_prevEdgeChanged == NOT_FOUND) {
    // initial enumration from a pure hypo
    for (size_t currEdge = 0 ; currEdge < sizePath ; currEdge++) {
      const Hypothesis	*hypo		= static_cast<const Hypothesis*>(m_path[currEdge]);
      const ArcList *pAL = hypo->GetArcList();
      if (!pAL) continue;
      const ArcList &arcList = *pAL;

      // every possible Arc to replace this edge
      ArcList::const_iterator iterArc;
      for (iterArc = arcList.begin() ; iterArc != arcList.end() ; ++iterArc) {
        const Hypothesis *arc = *iterArc;
        TrellisPath *deviantPath = new TrellisPath(*this, currEdge, arc);
        pathColl.Add(deviantPath);
      }
    }
  } else {
    // wiggle 1 of the edges only
    for (size_t currEdge = m_prevEdgeChanged + 1 ; currEdge < sizePath ; currEdge++) {
      const ArcList *pAL = m_path[currEdge]->GetArcList();
      if (!pAL) continue;
      const ArcList &arcList = *pAL;
      ArcList::const_iterator iterArc;

      for (iterArc = arcList.begin() ; iterArc != arcList.end() ; ++iterArc) {
        // copy this Path & change 1 edge
        const Hypothesis *arcReplace = *iterArc;

        TrellisPath *deviantPath = new TrellisPath(*this, currEdge, arcReplace);
        pathColl.Add(deviantPath);
      } // for (iterArc...
    } // for (currEdge = 0 ...
  }
}

boost::shared_ptr<ScoreComponentCollection> const
TrellisPath::
GetScoreBreakdown() const
{
  if (!m_scoreBreakdown) {
    m_scoreBreakdown.reset(new ScoreComponentCollection());
    m_scoreBreakdown->PlusEquals(m_path[0]->GetWinningHypo()->GetScoreBreakdown());

    // adjust score
    // I assume things are done this way on the assumption that most hypothesis edges
    // are shared with the winning path, so that score adjustments are cheaper than
    // recomputing the score from scratch. UG
    size_t sizePath = m_path.size();
    for (size_t pos = 0 ; pos < sizePath ; pos++) {
      const Hypothesis *hypo = m_path[pos];
      const Hypothesis *winningHypo = hypo->GetWinningHypo();
      if (hypo != winningHypo) {
        m_scoreBreakdown->MinusEquals(winningHypo->GetScoreBreakdown());
        m_scoreBreakdown->PlusEquals(hypo->GetScoreBreakdown());
      }
    }
  }

  return m_scoreBreakdown;
}

Phrase TrellisPath::GetTargetPhrase() const
{
  Phrase targetPhrase(ARRAY_SIZE_INCR);

  int numHypo = (int) m_path.size();
  for (int node = numHypo - 2 ; node >= 0 ; --node) {
    // don't do the empty hypo - waste of time and decode step id is invalid
    const Hypothesis &hypo = *m_path[node];
    const Phrase &currTargetPhrase = hypo.GetCurrTargetPhrase();

    targetPhrase.Append(currTargetPhrase);
  }

  return targetPhrase;
}

Phrase TrellisPath::GetSurfacePhrase() const
{
  std::vector<FactorType> const& oFactor = manager().options()->output.factor_order;
  Phrase targetPhrase = GetTargetPhrase();
  Phrase ret(targetPhrase.GetSize());

  for (size_t pos = 0 ; pos < targetPhrase.GetSize() ; ++pos) {
    Word &newWord = ret.AddWord();
    for (size_t i = 0 ; i < oFactor.size() ; i++) {
      FactorType factorType = oFactor[i];
      const Factor *factor = targetPhrase.GetFactor(pos, factorType);
      UTIL_THROW_IF2(factor == NULL,
                     "No factor " << factorType << " at position " << pos);
      newWord[factorType] = factor;
    }
  }

  return ret;
}

Range TrellisPath::GetTargetWordsRange(const Hypothesis &hypo) const
{
  size_t startPos = 0;

  for (int indEdge = (int) m_path.size() - 1 ; indEdge >= 0 ; --indEdge) {
    const Hypothesis *currHypo = m_path[indEdge];
    size_t endPos = startPos + currHypo->GetCurrTargetLength() - 1;

    if (currHypo == &hypo) {
      return Range(startPos, endPos);
    }
    startPos = endPos + 1;
  }

  // have to give a hypo in the trellis path, but u didn't.
  UTIL_THROW(util::Exception, "Hypothesis not found");
  return Range(NOT_FOUND, NOT_FOUND);
}

TO_STRING_BODY(TrellisPath);


}

