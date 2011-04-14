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

#include <vector>
#include <map>
#include <queue>
#include <set>
#include <iostream>
#include "WordsRange.h"
#include "Word.h"
#include "ChartHypothesis.h"

namespace Moses
{
class CoveredChartSpan;
class ChartTranslationOption;
extern bool g_debug;
class TranslationOptionCollection;
class TranslationOptionList;
class ChartCell;
class ChartCellCollection;
class RuleCube;
class RuleCubeQueue;

typedef std::vector<const ChartHypothesis*> HypoList;

// wrapper around list of hypothese for a particular non-term of a trans opt
class RuleCubeDimension
{
  friend std::ostream& operator<<(std::ostream&, const RuleCubeDimension&);

protected:
  size_t m_pos;
  const HypoList *m_orderedHypos;

public:
  RuleCubeDimension(size_t pos, const HypoList &orderedHypos)
    :m_pos(pos)
    ,m_orderedHypos(&orderedHypos)
  {}

  size_t IncrementPos() {
    return m_pos++;
  }

  bool HasMoreHypo() const {
    return m_pos + 1 < m_orderedHypos->size();
  }

  const ChartHypothesis *GetHypothesis() const {
    return (*m_orderedHypos)[m_pos];
  }

  //! transitive comparison used for adding objects into FactorCollection
  bool operator<(const RuleCubeDimension &compare) const {
    return GetHypothesis() < compare.GetHypothesis();
  }

  bool operator==(const RuleCubeDimension & compare) const {
    return GetHypothesis() == compare.GetHypothesis();
  }
};

// Stores one dimension in the cube
// (all the hypotheses that match one non terminal)
class RuleCube
{
  friend std::ostream& operator<<(std::ostream&, const RuleCube&);
protected:
  const ChartTranslationOption &m_transOpt;
  std::vector<RuleCubeDimension> m_cube;

  float m_combinedScore;

  RuleCube(const RuleCube &copy, size_t ruleCubeDimensionIncr);
  void CreateRuleCubeDimension(const CoveredChartSpan *coveredChartSpan, const ChartCellCollection &allChartCells);

  void CalcScore();

public:
  RuleCube(const ChartTranslationOption &transOpt
             , const ChartCellCollection &allChartCells);
  ~RuleCube();

  const ChartTranslationOption &GetTranslationOption() const {
    return m_transOpt;
  }
  const std::vector<RuleCubeDimension> &GetCube() const {
    return m_cube;
  }
  float GetCombinedScore() const {
    return m_combinedScore;
  }

  void CreateNeighbors(RuleCubeQueue &) const;

  bool operator<(const RuleCube &compare) const;

};

#ifdef HAVE_BOOST
std::size_t hash_value(const RuleCubeDimension &);
#endif

}
