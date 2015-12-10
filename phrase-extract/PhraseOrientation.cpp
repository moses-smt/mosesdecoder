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

#include "PhraseOrientation.h"

#include <iostream>
#include <sstream>
#include <limits>
#include <cassert>

#include <boost/assign/list_of.hpp>

namespace MosesTraining
{

std::vector<float> PhraseOrientation::m_l2rOrientationPriorCounts = boost::assign::list_of(0)(0)(0)(0)(0);
std::vector<float> PhraseOrientation::m_r2lOrientationPriorCounts = boost::assign::list_of(0)(0)(0)(0)(0);

PhraseOrientation::PhraseOrientation(int sourceSize,
                                     int targetSize,
                                     const Alignment &alignment)
  : m_countF(sourceSize)
  , m_countE(targetSize)
{
  // prepare data structures for alignments
  std::vector<std::vector<int> > alignedToS;
  for(int i=0; i<m_countF; ++i) {
    std::vector< int > dummy;
    alignedToS.push_back(dummy);
  }
  for(int i=0; i<m_countE; ++i) {
    std::vector< int > dummy;
    m_alignedToT.push_back(dummy);
  }
  std::vector<int> alignedCountS(m_countF,0);

  for (Alignment::const_iterator a=alignment.begin(); a!=alignment.end(); ++a) {
    alignedToS[a->first].push_back(a->second);
    alignedCountS[a->first]++;
    m_alignedToT[a->second].push_back(a->first);
  }

  Init(sourceSize, targetSize, m_alignedToT, alignedToS, alignedCountS);
}


PhraseOrientation::PhraseOrientation(int sourceSize,
                                     int targetSize,
                                     const Moses::AlignmentInfo &alignTerm,
                                     const Moses::AlignmentInfo &alignNonTerm)
  : m_countF(sourceSize)
  , m_countE(targetSize)
{
  // prepare data structures for alignments
  std::vector<std::vector<int> > alignedToS;
  for(int i=0; i<m_countF; ++i) {
    std::vector< int > dummy;
    alignedToS.push_back(dummy);
  }
  for(int i=0; i<m_countE; ++i) {
    std::vector< int > dummy;
    m_alignedToT.push_back(dummy);
  }
  std::vector<int> alignedCountS(m_countF,0);

  for (Moses::AlignmentInfo::const_iterator it=alignTerm.begin();
       it!=alignTerm.end(); ++it) {
    alignedToS[it->first].push_back(it->second);
    alignedCountS[it->first]++;
    m_alignedToT[it->second].push_back(it->first);
  }

  for (Moses::AlignmentInfo::const_iterator it=alignNonTerm.begin();
       it!=alignNonTerm.end(); ++it) {
    alignedToS[it->first].push_back(it->second);
    alignedCountS[it->first]++;
    m_alignedToT[it->second].push_back(it->first);
  }

  Init(sourceSize, targetSize, m_alignedToT, alignedToS, alignedCountS);
}

PhraseOrientation::PhraseOrientation(int sourceSize,
                                     int targetSize,
                                     const std::vector<std::vector<int> > &alignedToT,
                                     const std::vector<std::vector<int> > &alignedToS,
                                     const std::vector<int> &alignedCountS)
  : m_countF(sourceSize)
  , m_countE(targetSize)
  , m_alignedToT(alignedToT)
{
  Init(sourceSize, targetSize, m_alignedToT, alignedToS, alignedCountS);
}


void PhraseOrientation::Init(int sourceSize,
                             int targetSize,
                             const std::vector<std::vector<int> > &alignedToT,
                             const std::vector<std::vector<int> > &alignedToS,
                             const std::vector<int> &alignedCountS)
{
  for (int startF=0; startF<m_countF; ++startF) {
    for (int endF=startF; endF<m_countF; ++endF) {

      int minE = std::numeric_limits<int>::max();
      int maxE = -1;
      for (int fi=startF; fi<=endF; ++fi) {
        for (size_t i=0; i<alignedToS[fi].size(); ++i) {
          int ei = alignedToS[fi][i];
          if (ei<minE) {
            minE = ei;
          }
          if (ei>maxE) {
            maxE = ei;
          }
        }
      }

      m_minAndMaxAlignedToSourceSpan[ std::pair<int,int>(startF,endF) ] = std::pair<int,int>(minE,maxE);
    }
  }

  // check alignments for target phrase startE...endE
  // loop over continuous phrases which are compatible with the word alignments
  for (int startE=0; startE<m_countE; ++startE) {
    for (int endE=startE; endE<m_countE; ++endE) {

      int minF = std::numeric_limits<int>::max();
      int maxF = -1;
      std::vector< int > usedF = alignedCountS;
      for (int ei=startE; ei<=endE; ++ei) {
        for (size_t i=0; i<alignedToT[ei].size(); ++i) {
          int fi = alignedToT[ei][i];
          if (fi<minF) {
            minF = fi;
          }
          if (fi>maxF) {
            maxF = fi;
          }
          usedF[fi]--;
        }
      }

      m_minAndMaxAlignedToTargetSpan[ std::pair<int,int>(startE,endE) ] = std::pair<int,int>(minF,maxF);

      if (maxF >= 0) { // aligned to any source words at all

        // check if source words are aligned to out of bounds target words
        bool out_of_bounds = false;
        for (int fi=minF; fi<=maxF && !out_of_bounds; ++fi)
          if (usedF[fi]>0) {
            // cout << "out of bounds: " << fi << "\n";
            out_of_bounds = true;
          }

        // cout << "doing if for ( " << minF << "-" << maxF << ", " << startE << "," << endE << ")\n";
        if (!out_of_bounds) {
          // start point of source phrase may retreat over unaligned
          for (int startF=minF;
               (startF>=0 &&
                (startF==minF || alignedCountS[startF]==0)); // unaligned
               startF--) {
            // end point of source phrase may advance over unaligned
            for (int endF=maxF;
                 (endF<m_countF &&
                  (endF==maxF || alignedCountS[endF]==0)); // unaligned
                 endF++) { // at this point we have extracted a phrase

              InsertPhraseVertices(m_topLeft, m_topRight, m_bottomLeft, m_bottomRight,
                                   startF, startE, endF, endE);
            }
          }
        }
      }
    }
  }
}


void PhraseOrientation::InsertVertex( HSentenceVertices & corners, int x, int y )
{
  std::set<int> tmp;
  tmp.insert(x);
  std::pair< HSentenceVertices::iterator, bool > ret = corners.insert( std::pair<int, std::set<int> > (y, tmp) );
  if (ret.second == false) {
    ret.first->second.insert(x);
  }
}


void PhraseOrientation::InsertPhraseVertices(HSentenceVertices & topLeft,
    HSentenceVertices & topRight,
    HSentenceVertices & bottomLeft,
    HSentenceVertices & bottomRight,
    int startF, int startE, int endF, int endE)
{

  InsertVertex(topLeft, startF, startE);
  InsertVertex(topRight, endF, startE);
  InsertVertex(bottomLeft, startF, endE);
  InsertVertex(bottomRight, endF, endE);
}


const std::string PhraseOrientation::GetOrientationInfoString(int startF, int endF, REO_DIR direction) const
{
  boost::unordered_map< std::pair<int,int> , std::pair<int,int> >::const_iterator foundMinMax
  = m_minAndMaxAlignedToSourceSpan.find( std::pair<int,int>(startF,endF) );

  if ( foundMinMax != m_minAndMaxAlignedToSourceSpan.end() ) {
    int startE = (foundMinMax->second).first;
    int endE   = (foundMinMax->second).second;
//    std::cerr << "Phrase orientation for"
//      << " startF=" << startF
//      << " endF="   << endF
//      << " startE=" << startE
//      << " endE="   << endE
//      << std::endl;
    return GetOrientationInfoString(startF, startE, endF, endE, direction);
  } else {
    std::cerr << "PhraseOrientation::GetOrientationInfoString(): Error: not able to determine phrase orientation" << std::endl;
    std::exit(1);
  }
}


const std::string PhraseOrientation::GetOrientationInfoString(int startF, int startE, int endF, int endE, REO_DIR direction) const
{
  REO_CLASS hierPrevOrient=REO_CLASS_UNKNOWN, hierNextOrient=REO_CLASS_UNKNOWN;

  if ( direction == REO_DIR_L2R || direction == REO_DIR_BIDIR )
    hierPrevOrient = GetOrientationInfo(startF, startE, endF, endE, REO_DIR_L2R);

  if ( direction == REO_DIR_R2L || direction == REO_DIR_BIDIR )
    hierNextOrient = GetOrientationInfo(startF, startE, endF, endE, REO_DIR_R2L);

  switch (direction) {
  case REO_DIR_L2R:
    return GetOrientationString(hierPrevOrient, REO_MODEL_TYPE_MSLR);
    break;
  case REO_DIR_R2L:
    return GetOrientationString(hierNextOrient, REO_MODEL_TYPE_MSLR);
    break;
  case REO_DIR_BIDIR:
    return GetOrientationString(hierPrevOrient, REO_MODEL_TYPE_MSLR) + " " + GetOrientationString(hierNextOrient, REO_MODEL_TYPE_MSLR);
    break;
  default:
    return GetOrientationString(hierPrevOrient, REO_MODEL_TYPE_MSLR) + " " + GetOrientationString(hierNextOrient, REO_MODEL_TYPE_MSLR);
    break;
  }
  return "PhraseOrientationERROR";
}


PhraseOrientation::REO_CLASS PhraseOrientation::GetOrientationInfo(int startF, int endF, REO_DIR direction) const
{
  boost::unordered_map< std::pair<int,int> , std::pair<int,int> >::const_iterator foundMinMax
  = m_minAndMaxAlignedToSourceSpan.find( std::pair<int,int>(startF,endF) );

  if ( foundMinMax != m_minAndMaxAlignedToSourceSpan.end() ) {
    int startE = (foundMinMax->second).first;
    int endE   = (foundMinMax->second).second;
//    std::cerr << "Phrase orientation for"
//      << " startF=" << startF
//      << " endF="   << endF
//      << " startE=" << startE
//      << " endE="   << endE
//      << std::endl;
    return GetOrientationInfo(startF, startE, endF, endE, direction);
  } else {
    std::cerr << "PhraseOrientation::GetOrientationInfo(): Error: not able to determine phrase orientation" << std::endl;
    std::exit(1);
  }
}


PhraseOrientation::REO_CLASS PhraseOrientation::GetOrientationInfo(int startF, int startE, int endF, int endE, REO_DIR direction) const
{
  if ( direction != REO_DIR_L2R && direction != REO_DIR_R2L ) {
    std::cerr << "PhraseOrientation::GetOrientationInfo(): Error: direction should be either L2R or R2L" << std::endl;
    std::exit(1);
  }

  if ( direction == REO_DIR_L2R )
    return GetOrientHierModel(REO_MODEL_TYPE_MSLR,
                              startF, endF, startE, endE, m_countF-1, 0, 0, 1,
                              &ge, &le,
                              m_bottomRight, m_bottomLeft);

  if ( direction == REO_DIR_R2L )
    return GetOrientHierModel(REO_MODEL_TYPE_MSLR,
                              endF, startF, endE, startE, 0, m_countF-1, m_countE-1, -1,
                              &le, &ge,
                              m_topLeft, m_topRight);

  return REO_CLASS_UNKNOWN;
}


// to be called with countF-1 instead of countF
PhraseOrientation::REO_CLASS PhraseOrientation::GetOrientHierModel(REO_MODEL_TYPE modelType,
    int startF, int endF, int startE, int endE, int countF, int zeroF, int zeroE, int unit,
    bool (*ge)(int, int), bool (*le)(int, int),
    const HSentenceVertices & bottomRight, const HSentenceVertices & bottomLeft) const
{
  bool leftSourceSpanIsAligned = ( (startF != zeroF) && SourceSpanIsAligned(zeroF,startF-unit) );
  bool topTargetSpanIsAligned  = ( (startE != zeroE) && TargetSpanIsAligned(zeroE,startE-unit) );

  if (!topTargetSpanIsAligned && !leftSourceSpanIsAligned)
    return REO_CLASS_LEFT;

  HSentenceVertices::const_iterator it;

  if (//(connectedLeftTop && !connectedRightTop) ||
    ((it = bottomRight.find(startE - unit)) != bottomRight.end() &&
     it->second.find(startF-unit) != it->second.end()))
    return REO_CLASS_LEFT;

  if (modelType == REO_MODEL_TYPE_MONO)
    return REO_CLASS_UNKNOWN;

  if (//(!connectedLeftTop &&  connectedRightTop) ||
    ((it = bottomLeft.find(startE - unit)) != bottomLeft.end() &&
     it->second.find(endF + unit) != it->second.end()))
    return REO_CLASS_RIGHT;

  if (modelType == REO_MODEL_TYPE_MSD)
    return REO_CLASS_UNKNOWN;

  for (int indexF=startF-2*unit; (*ge)(indexF, zeroF); indexF=indexF-unit) {
    if ((it = bottomRight.find(startE - unit)) != bottomRight.end() &&
        it->second.find(indexF) != it->second.end())
      return REO_CLASS_DLEFT;
  }

  for (int indexF=endF+2*unit; (*le)(indexF, countF); indexF=indexF+unit) {
    if ((it = bottomLeft.find(startE - unit)) != bottomLeft.end() &&
        it->second.find(indexF) != it->second.end())
      return REO_CLASS_DRIGHT;
  }

  return REO_CLASS_UNKNOWN;
}

bool PhraseOrientation::SourceSpanIsAligned(int index1, int index2) const
{
  return SpanIsAligned(index1, index2, m_minAndMaxAlignedToSourceSpan);
}

bool PhraseOrientation::TargetSpanIsAligned(int index1, int index2) const
{
  return SpanIsAligned(index1, index2, m_minAndMaxAlignedToTargetSpan);
}

bool PhraseOrientation::SpanIsAligned(int index1, int index2, const boost::unordered_map< std::pair<int,int> , std::pair<int,int> > &minAndMaxAligned) const
{
  boost::unordered_map< std::pair<int,int> , std::pair<int,int> >::const_iterator itMinAndMaxAligned =
    minAndMaxAligned.find(std::pair<int,int>(std::min(index1,index2),std::max(index1,index2)));

  if (itMinAndMaxAligned == minAndMaxAligned.end()) {
    std::cerr << "PhraseOrientation::SourceSpanIsAligned(): Error" << std::endl;
    std::exit(1);
  } else {
    if (itMinAndMaxAligned->second.first == std::numeric_limits<int>::max()) {
      return false;
    }
  }
  return true;
}


const std::string PhraseOrientation::GetOrientationString(const REO_CLASS orient, const REO_MODEL_TYPE modelType)
{
  std::ostringstream oss;
  WriteOrientation(oss, orient, modelType);
  return oss.str();
}


void PhraseOrientation::WriteOrientation(std::ostream& out, const REO_CLASS orient, const REO_MODEL_TYPE modelType)
{
  switch(orient) {
  case REO_CLASS_LEFT:
    out << "mono";
    break;
  case REO_CLASS_RIGHT:
    out << "swap";
    break;
  case REO_CLASS_DLEFT:
    out << "dleft";
    break;
  case REO_CLASS_DRIGHT:
    out << "dright";
    break;
  case REO_CLASS_UNKNOWN:
    switch(modelType) {
    case REO_MODEL_TYPE_MONO:
      out << "nomono";
      break;
    case REO_MODEL_TYPE_MSD:
      out << "other";
      break;
    case REO_MODEL_TYPE_MSLR:
      out << "dleft";
      break;
    }
    break;
  }
}


bool PhraseOrientation::IsAligned(int fi, int ei) const
{
  if (ei == -1 && fi == -1)
    return true;

  if (ei <= -1 || fi <= -1)
    return false;

  if (ei == m_countE && fi == m_countF)
    return true;

  if (ei >= m_countE || fi >= m_countF)
    return false;

  for (size_t i=0; i<m_alignedToT[ei].size(); ++i)
    if (m_alignedToT[ei][i] == fi)
      return true;

  return false;
}


void PhraseOrientation::IncrementPriorCount(REO_DIR direction, REO_CLASS orient, float increment)
{
  assert(direction==REO_DIR_L2R || direction==REO_DIR_R2L);
  if (direction == REO_DIR_L2R) {
    m_l2rOrientationPriorCounts[orient] += increment;
  } else if (direction == REO_DIR_R2L) {
    m_r2lOrientationPriorCounts[orient] += increment;
  }
}


void PhraseOrientation::WritePriorCounts(std::ostream& out, const REO_MODEL_TYPE modelType)
{
  std::map<std::string,float> l2rOrientationPriorCountsMap;
  std::map<std::string,float> r2lOrientationPriorCountsMap;
  for (int orient=0; orient<=REO_CLASS_UNKNOWN; ++orient) {
    l2rOrientationPriorCountsMap[GetOrientationString((REO_CLASS)orient, modelType)] += m_l2rOrientationPriorCounts[orient];
  }
  for (int orient=0; orient<=REO_CLASS_UNKNOWN; ++orient) {
    r2lOrientationPriorCountsMap[GetOrientationString((REO_CLASS)orient, modelType)] += m_r2lOrientationPriorCounts[orient];
  }
  for (std::map<std::string,float>::const_iterator l2rOrientationPriorCountsMapIt = l2rOrientationPriorCountsMap.begin();
       l2rOrientationPriorCountsMapIt != l2rOrientationPriorCountsMap.end(); ++l2rOrientationPriorCountsMapIt) {
    out << "L2R_" << l2rOrientationPriorCountsMapIt->first << " " << l2rOrientationPriorCountsMapIt->second << std::endl;
  }
  for (std::map<std::string,float>::const_iterator r2lOrientationPriorCountsMapIt = r2lOrientationPriorCountsMap.begin();
       r2lOrientationPriorCountsMapIt != r2lOrientationPriorCountsMap.end(); ++r2lOrientationPriorCountsMapIt) {
    out << "R2L_" << r2lOrientationPriorCountsMapIt->first << " " << r2lOrientationPriorCountsMapIt->second << std::endl;
  }
}

}  // namespace MosesTraining
