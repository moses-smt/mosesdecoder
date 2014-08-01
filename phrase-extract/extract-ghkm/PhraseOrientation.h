
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

#include "Alignment.h"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>

namespace Moses
{
namespace GHKM
{

enum REO_MODEL_TYPE {REO_MSD, REO_MSLR, REO_MONO};
enum REO_POS {LEFT, RIGHT, DLEFT, DRIGHT, UNKNOWN};
enum REO_DIR {L2R, R2L, BIDIR};

// The key of the map is the English index and the value is a set of the source ones
typedef std::map <int, std::set<int> > HSentenceVertices;


class PhraseOrientation
{
public:

  PhraseOrientation(const std::vector<std::string> &source,
                    const std::vector<std::string> &target,
                    const Alignment &alignment);

  REO_POS GetOrientationInfo(int startF, int endF, REO_DIR direction) const;
  REO_POS GetOrientationInfo(int startF, int startE, int endF, int endE, REO_DIR direction) const;
  const std::string GetOrientationInfoString(int startF, int endF, REO_DIR direction=BIDIR) const;
  const std::string GetOrientationInfoString(int startF, int startE, int endF, int endE, REO_DIR direction=BIDIR) const;
  static const std::string GetOrientationString(const REO_POS orient, const REO_MODEL_TYPE modelType=REO_MSLR);
  static void WriteOrientation(std::ostream& out, const REO_POS orient, const REO_MODEL_TYPE modelType=REO_MSLR);
  void IncrementPriorCount(REO_DIR direction, REO_POS orient, float increment);
  static void WritePriorCounts(std::ostream& out, const REO_MODEL_TYPE modelType=REO_MSLR);

private:

  void InsertVertex( HSentenceVertices & corners, int x, int y );

  void InsertPhraseVertices(HSentenceVertices & topLeft,
                            HSentenceVertices & topRight,
                            HSentenceVertices & bottomLeft,
                            HSentenceVertices & bottomRight,
                            int startF, int startE, int endF, int endE);

  REO_POS GetOrientHierModel(REO_MODEL_TYPE modelType,
                             bool connectedLeftTop, bool connectedRightTop,
                             int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                             bool (*ge)(int, int), bool (*lt)(int, int),
                             const HSentenceVertices & bottomRight, const HSentenceVertices & bottomLeft) const;

  bool IsAligned(int fi, int ei) const;

  static bool ge(int first, int second) { return first >= second; };
  static bool le(int first, int second) { return first <= second; };
  static bool lt(int first, int second) { return first < second; };

  const std::vector<std::string> &m_source;
  const std::vector<std::string> &m_target;
  const Alignment &m_alignment;

  std::vector<std::vector<int> > m_alignedToT;

  HSentenceVertices m_topLeft;
  HSentenceVertices m_topRight;
  HSentenceVertices m_bottomLeft;
  HSentenceVertices m_bottomRight;

  boost::unordered_map< std::pair<int,int> , std::pair<int,int> > m_minAndMaxAlignedToSourceSpan;

  static std::vector<float> m_l2rOrientationPriorCounts;
  static std::vector<float> m_r2lOrientationPriorCounts;
};

}  // namespace GHKM
}  // namespace Moses

