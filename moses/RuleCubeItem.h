/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

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

#include "StackVec.h"
#include "ChartTranslationOptions.h"
#include <vector>

namespace Moses
{

class ChartCellCollection;
class ChartHypothesis;
class ChartManager;
class TargetPhrase;

typedef std::vector<const ChartHypothesis*> HypoList;

/** wrapper around list of target phrase translation options
 * @todo How is this used. Split out into separate source file
 */
class TranslationDimension
{
public:
  TranslationDimension(std::size_t pos,
                       const ChartTranslationOptions::CollType &orderedTargetPhrases)
    : m_pos(pos)
    , m_orderedTargetPhrases(orderedTargetPhrases) {
  }

  std::size_t IncrementPos() {
    return m_pos++;
  }

  bool HasMoreTranslations() const {
    return m_pos+1 < m_orderedTargetPhrases.size();
  }

  const boost::shared_ptr<ChartTranslationOption> &GetTranslationOption() const {
    return m_orderedTargetPhrases[m_pos];
  }

  bool operator<(const TranslationDimension &compare) const {
    return GetTranslationOption()->GetPhrase() < compare.GetTranslationOption()->GetPhrase();
  }

  bool operator==(const TranslationDimension &compare) const {
    return GetTranslationOption()->GetPhrase() == compare.GetTranslationOption()->GetPhrase();
  }

private:
  std::size_t m_pos;
  const ChartTranslationOptions::CollType &m_orderedTargetPhrases;
};


/** wrapper around list of hypotheses for a particular non-term of a trans opt
 * @todo How is this used. Split out into separate source file
 */
class HypothesisDimension
{
public:
  HypothesisDimension(std::size_t pos, const HypoList &orderedHypos)
    : m_pos(pos)
    , m_orderedHypos(&orderedHypos) {
  }

  std::size_t IncrementPos() {
    return m_pos++;
  }

  bool HasMoreHypo() const {
    return m_pos+1 < m_orderedHypos->size();
  }

  const ChartHypothesis *GetHypothesis() const {
    return (*m_orderedHypos)[m_pos];
  }

  bool operator<(const HypothesisDimension &compare) const {
    return GetHypothesis() < compare.GetHypothesis();
  }

  bool operator==(const HypothesisDimension &compare) const {
    return GetHypothesis() == compare.GetHypothesis();
  }

private:
  std::size_t m_pos;
  const HypoList *m_orderedHypos;
};

std::size_t hash_value(const HypothesisDimension &);

/** @todo How is this used. Split out into separate source file */
class RuleCubeItem
{
public:
  RuleCubeItem(const ChartTranslationOptions &, const ChartCellCollection &);
  RuleCubeItem(const RuleCubeItem &, int);
  ~RuleCubeItem();

  const TranslationDimension &GetTranslationDimension() const {
    return m_translationDimension;
  }

  const std::vector<HypothesisDimension> &GetHypothesisDimensions() const {
    return m_hypothesisDimensions;
  }

  float GetScore() const {
    return m_score;
  }

  void EstimateScore();

  void CreateHypothesis(const ChartTranslationOptions &, ChartManager &);

  ChartHypothesis *ReleaseHypothesis();

  bool operator<(const RuleCubeItem &) const;

private:
  RuleCubeItem(const RuleCubeItem &);  // Not implemented
  RuleCubeItem &operator=(const RuleCubeItem &);  // Not implemented

  void CreateHypothesisDimensions(const StackVec &);

  TranslationDimension m_translationDimension;
  std::vector<HypothesisDimension> m_hypothesisDimensions;
  ChartHypothesis *m_hypothesis;
  float m_score;
};

}
