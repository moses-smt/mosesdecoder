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

#ifndef moses_TrellisPath_h
#define moses_TrellisPath_h

#include <iostream>
#include <vector>
#include <limits>
#include "Hypothesis.h"
#include "TypeDef.h"

namespace Moses
{

class TrellisPathCollection;
class TrellisPathList;

/** Encapsulate the set of hypotheses/arcs that goes from decoding 1 phrase to all the source phrases
 *	to reach a final translation. For the best translation, this consist of all hypotheses, for the other
 *	n-best paths, the node on the path can consist of hypotheses or arcs.
 *  Used by phrase-based decoding
 */
class TrellisPath
{
  friend std::ostream& operator<<(std::ostream&, const TrellisPath&);
  friend class Manager;

protected:
  std::vector<const Hypothesis *> m_path; //< list of hypotheses/arcs
  size_t		m_prevEdgeChanged; /**< the last node that was wiggled to create this path
																	, or NOT_FOUND if this path is the best trans so consist of only hypos
															 */

  ScoreComponentCollection	m_scoreBreakdown;
  float m_totalScore;

  //Used by Manager::LatticeSample()
  TrellisPath(const std::vector<const Hypothesis*> edges);

  void InitScore();

public:
  TrellisPath(); // not implemented

  //! create path OF pure hypo
  TrellisPath(const Hypothesis *hypo);

  /** create path from another path, deviate at edgeIndex by using arc instead,
  	* which may change other hypo back from there
  	*/
  TrellisPath(const TrellisPath &copy, size_t edgeIndex, const Hypothesis *arc);

  //! get score for this path throught trellis
  inline float GetTotalScore() const {
    return m_totalScore;
  }

  /** list of each hypo/arcs in path. For anything other than the best hypo, it is not possible just to follow the
  	* m_prevHypo variable in the hypothesis object
  	*/
  inline const std::vector<const Hypothesis *> &GetEdges() const {
    return m_path;
  }

  inline size_t GetSize() const {
    return m_path.size();
  }

  //! create a set of next best paths by wiggling 1 of the node at a time.
  void CreateDeviantPaths(TrellisPathCollection &pathColl) const;

  //! create a list of next best paths by wiggling 1 of the node at a time.
  void CreateDeviantPaths(TrellisPathList &pathColl) const;

  inline const ScoreComponentCollection &GetScoreBreakdown() const {
    return m_scoreBreakdown;
  }

  //! get target words range of the hypo within n-best trellis. not necessarily the same as hypo.GetCurrTargetWordsRange()
  WordsRange GetTargetWordsRange(const Hypothesis &hypo) const;

  Phrase GetTargetPhrase() const;
  Phrase GetSurfacePhrase() const;

  TO_STRING();

};

// friend
inline std::ostream& operator<<(std::ostream& out, const TrellisPath& path)
{
  const size_t sizePath = path.m_path.size();
  for (int pos = (int) sizePath - 1 ; pos >= 0 ; pos--) {
    const Hypothesis *edge = path.m_path[pos];
    const WordsRange &sourceRange = edge->GetCurrSourceWordsRange();
    out << edge->GetId() << " " << sourceRange.GetStartPos() << "-" << sourceRange.GetEndPos() << ", ";
  }
  // scores
  out << " total=" << path.GetTotalScore()
      << " " << path.GetScoreBreakdown()
      << std::endl;

  return out;
}

}
#endif
