
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
#include "moses/AlignmentInfo.h"

#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/unordered_map.hpp>

namespace Moses
{
namespace GHKM
{

// The key of the map is the English index and the value is a set of the source ones
typedef std::map <int, std::set<int> > HSentenceVertices;


class PhraseOrientation
{
public:

  enum REO_MODEL_TYPE {REO_MODEL_TYPE_MSD, REO_MODEL_TYPE_MSLR, REO_MODEL_TYPE_MONO};
  enum REO_CLASS {REO_CLASS_LEFT, REO_CLASS_RIGHT, REO_CLASS_DLEFT, REO_CLASS_DRIGHT, REO_CLASS_UNKNOWN};
  enum REO_DIR {REO_DIR_L2R, REO_DIR_R2L, REO_DIR_BIDIR};


  PhraseOrientation(int sourceSize,
                    int targetSize,
                    const Alignment &alignment);

  PhraseOrientation(int sourceSize,
                    int targetSize,
                    const AlignmentInfo &alignTerm,
                    const AlignmentInfo &alignNonTerm);

  REO_CLASS GetOrientationInfo(int startF, int endF, REO_DIR direction) const;
  REO_CLASS GetOrientationInfo(int startF, int startE, int endF, int endE, REO_DIR direction) const;
  const std::string GetOrientationInfoString(int startF, int endF, REO_DIR direction=REO_DIR_BIDIR) const;
  const std::string GetOrientationInfoString(int startF, int startE, int endF, int endE, REO_DIR direction=REO_DIR_BIDIR) const;
  static const std::string GetOrientationString(const REO_CLASS orient, const REO_MODEL_TYPE modelType=REO_MODEL_TYPE_MSLR);
  static void WriteOrientation(std::ostream& out, const REO_CLASS orient, const REO_MODEL_TYPE modelType=REO_MODEL_TYPE_MSLR);
  void IncrementPriorCount(REO_DIR direction, REO_CLASS orient, float increment);
  static void WritePriorCounts(std::ostream& out, const REO_MODEL_TYPE modelType=REO_MODEL_TYPE_MSLR);
  bool SourceSpanIsAligned(int index1, int index2) const;
  bool TargetSpanIsAligned(int index1, int index2) const;

private:

  void Init(int sourceSize, int targetSize,
            const std::vector<std::vector<int> > &alignedToT,
            const std::vector<std::vector<int> > &alignedToS,
            const std::vector<int> &alignedCountS);

  void InsertVertex( HSentenceVertices & corners, int x, int y );

  void InsertPhraseVertices(HSentenceVertices & topLeft,
                            HSentenceVertices & topRight,
                            HSentenceVertices & bottomLeft,
                            HSentenceVertices & bottomRight,
                            int startF, int startE, int endF, int endE);

  REO_CLASS GetOrientHierModel(REO_MODEL_TYPE modelType,
                             int startF, int endF, int startE, int endE, int countF, int zeroF, int zeroE, int unit,
                             bool (*ge)(int, int), bool (*lt)(int, int),
                             const HSentenceVertices & bottomRight, const HSentenceVertices & bottomLeft) const;

  bool SpanIsAligned(int index1, int index2, const boost::unordered_map< std::pair<int,int> , std::pair<int,int> > &minAndMaxAligned) const;

  bool IsAligned(int fi, int ei) const;

  static bool ge(int first, int second) { return first >= second; };
  static bool le(int first, int second) { return first <= second; };
  static bool lt(int first, int second) { return first < second; };

  const int m_countF;
  const int m_countE;

  std::vector<std::vector<int> > m_alignedToT;

  HSentenceVertices m_topLeft;
  HSentenceVertices m_topRight;
  HSentenceVertices m_bottomLeft;
  HSentenceVertices m_bottomRight;

  boost::unordered_map< std::pair<int,int> , std::pair<int,int> > m_minAndMaxAlignedToSourceSpan;
  boost::unordered_map< std::pair<int,int> , std::pair<int,int> > m_minAndMaxAlignedToTargetSpan;

  static std::vector<float> m_l2rOrientationPriorCounts;
  static std::vector<float> m_r2lOrientationPriorCounts;
};

}  // namespace GHKM
}  // namespace Moses

