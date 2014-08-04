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

#include <sstream>
#include <iostream>
#include <sstream>
#include <limits>

#include <boost/assign/list_of.hpp>

namespace Moses
{
namespace GHKM
{

std::vector<float> PhraseOrientation::m_l2rOrientationPriorCounts = boost::assign::list_of(0)(0)(0)(0)(0);
std::vector<float> PhraseOrientation::m_r2lOrientationPriorCounts = boost::assign::list_of(0)(0)(0)(0)(0);

PhraseOrientation::PhraseOrientation(const std::vector<std::string> &source,
                                     const std::vector<std::string> &target,
                                     const Alignment &alignment)
  : m_source(source)
  , m_target(target)
  , m_alignment(alignment)
{

  int countF = m_source.size();
  int countE = m_target.size();

  // prepare data structures for alignments
  std::vector<std::vector<int> > alignedToS;
  for(int i=0; i<countF; ++i) {
    std::vector< int > dummy;
    alignedToS.push_back(dummy);
  }
  for(int i=0; i<countE; ++i) {
    std::vector< int > dummy;
    m_alignedToT.push_back(dummy);
  }
  std::vector<int> alignedCountS(countF,0);

  for (Alignment::const_iterator a=alignment.begin(); a!=alignment.end(); ++a) {
    m_alignedToT[a->second].push_back(a->first);
    alignedCountS[a->first]++;
    alignedToS[a->first].push_back(a->second);
  }

  for (int startF=0; startF<countF; ++startF) {
    for (int endF=startF; endF<countF; ++endF) {

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
  for (int startE=0; startE<countE; ++startE) {
    for (int endE=startE; endE<countE; ++endE) {

      int minF = std::numeric_limits<int>::max();
      int maxF = -1;
      std::vector< int > usedF = alignedCountS;
      for (int ei=startE; ei<=endE; ++ei) {
        for (size_t i=0; i<m_alignedToT[ei].size(); ++i) {
          int fi = m_alignedToT[ei][i];
          if (fi<minF) {
            minF = fi;
          }
          if (fi>maxF) {
            maxF = fi;
          }
          usedF[fi]--;
        }
      }

      if (maxF >= 0) { // aligned to any source words at all

        // check if source words are aligned to out of bound target words
        bool out_of_bounds = false;
        for (int fi=minF; fi<=maxF && !out_of_bounds; ++fi)
          if (usedF[fi]>0) {
            // cout << "ouf of bounds: " << fi << "\n";
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
                 (endF<countF &&
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
    std::cerr << "Error: not able to determine phrase orientation" << std::endl;
    std::exit(1);
  }
}


const std::string PhraseOrientation::GetOrientationInfoString(int startF, int startE, int endF, int endE, REO_DIR direction) const
{
  REO_POS hierPrevOrient, hierNextOrient;

  bool connectedLeftTopP  = IsAligned( startF-1, startE-1 );
  bool connectedRightTopP = IsAligned( endF+1,   startE-1 );
  bool connectedLeftTopN  = IsAligned( endF+1,   endE+1   );
  bool connectedRightTopN = IsAligned( startF-1, endE+1   );

  if ( direction == L2R || direction == BIDIR )
    hierPrevOrient = GetOrientHierModel(REO_MSLR,
                                        connectedLeftTopP, connectedRightTopP,
                                        startF, endF, startE, endE, m_source.size()-1, 0, 1, 
                                        &ge, &lt, 
                                        m_bottomRight, m_bottomLeft);

  if ( direction == R2L || direction == BIDIR )
    hierNextOrient = GetOrientHierModel(REO_MSLR,
                                        connectedLeftTopN, connectedRightTopN,
                                        endF, startF, endE, startE, 0, m_source.size()-1, -1, 
                                        &lt, &ge, 
                                        m_bottomLeft, m_bottomRight); 

  switch (direction) {
    case L2R:
      return GetOrientationString(hierPrevOrient, REO_MSLR);
      break;
    case R2L:
      return GetOrientationString(hierNextOrient, REO_MSLR);
      break;
    case BIDIR:
      return GetOrientationString(hierPrevOrient, REO_MSLR) + " " + GetOrientationString(hierNextOrient, REO_MSLR);
      break;
    default:
      return GetOrientationString(hierPrevOrient, REO_MSLR) + " " + GetOrientationString(hierNextOrient, REO_MSLR);
      break;
  }
  return "PhraseOrientationERROR";
}


REO_POS PhraseOrientation::GetOrientationInfo(int startF, int endF, REO_DIR direction) const
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
    std::cerr << "Error: not able to determine phrase orientation" << std::endl;
    std::exit(1);
  }
}


REO_POS PhraseOrientation::GetOrientationInfo(int startF, int startE, int endF, int endE, REO_DIR direction) const
{
  if ( direction != L2R && direction != R2L ) {
    std::cerr << "PhraseOrientation::GetOrientationInfo(): direction should be either L2R or R2L" << std::endl;
    std::exit(1);
  }

  bool connectedLeftTopP  = IsAligned( startF-1, startE-1 );
  bool connectedRightTopP = IsAligned( endF+1,   startE-1 );
  bool connectedLeftTopN  = IsAligned( endF+1,   endE+1   );
  bool connectedRightTopN = IsAligned( startF-1, endE+1   );

  if ( direction == L2R )
    return GetOrientHierModel(REO_MSLR,
                              connectedLeftTopP, connectedRightTopP,
                              startF, endF, startE, endE, m_source.size()-1, 0, 1, 
                              &ge, &lt, 
                              m_bottomRight, m_bottomLeft);

  if ( direction == R2L )
    return GetOrientHierModel(REO_MSLR,
                              connectedLeftTopN, connectedRightTopN,
                              endF, startF, endE, startE, 0, m_source.size()-1, -1, 
                              &lt, &ge, 
                              m_bottomLeft, m_bottomRight);

  return UNKNOWN; 
}


// to be called with countF-1 instead of countF
REO_POS PhraseOrientation::GetOrientHierModel(REO_MODEL_TYPE modelType,
                                              bool connectedLeftTop, bool connectedRightTop,
                                              int startF, int endF, int startE, int endE, int countF, int zero, int unit,
                                              bool (*ge)(int, int), bool (*lt)(int, int),
                                              const HSentenceVertices & bottomRight, const HSentenceVertices & bottomLeft) const
{
  HSentenceVertices::const_iterator it;

  if ((connectedLeftTop && !connectedRightTop) ||
      ((it = bottomRight.find(startE - unit)) != bottomRight.end() &&
       it->second.find(startF-unit) != it->second.end()))
    return LEFT;

  if (modelType == REO_MONO)
    return UNKNOWN;

  if ((!connectedLeftTop &&  connectedRightTop) ||
      ((it = bottomLeft.find(startE - unit)) != bottomLeft.end() &&
       it->second.find(endF + unit) != it->second.end()))
    return RIGHT;

  if (modelType == REO_MSD)
    return UNKNOWN;

  connectedLeftTop = false;
  for (int indexF=startF-2*unit; (*ge)(indexF, zero) && !connectedLeftTop; indexF=indexF-unit) {
    if ((connectedLeftTop = ((it = bottomRight.find(startE - unit)) != bottomRight.end() &&
                             it->second.find(indexF) != it->second.end())))
      return DRIGHT;
  }

  connectedRightTop = false;
  for (int indexF=endF+2*unit; (*lt)(indexF, countF) && !connectedRightTop; indexF=indexF+unit) {
    if ((connectedRightTop = ((it = bottomLeft.find(startE - unit)) != bottomLeft.end() &&
                              it->second.find(indexF) != it->second.end())))
      return DLEFT;
  }

  return UNKNOWN;
}


const std::string PhraseOrientation::GetOrientationString(const REO_POS orient, const REO_MODEL_TYPE modelType)
{
  std::ostringstream oss;
  WriteOrientation(oss, orient, modelType);
  return oss.str();
}


void PhraseOrientation::WriteOrientation(std::ostream& out, const REO_POS orient, const REO_MODEL_TYPE modelType)
{
  switch(orient) {
  case LEFT:
    out << "mono";
    break;
  case RIGHT:
    out << "swap";
    break;
  case DRIGHT:
    out << "dright";
    break;
  case DLEFT:
    out << "dleft";
    break;
  case UNKNOWN:
    switch(modelType) {
    case REO_MONO:
      out << "nomono";
      break;
    case REO_MSD:
      out << "other";
      break;
    case REO_MSLR:
      out << "dright";
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

  if (ei == (int)m_target.size() && fi == (int)m_source.size())
    return true;

  if (ei >= (int)m_target.size() || fi >= (int)m_source.size())
    return false;

  for (size_t i=0; i<m_alignedToT[ei].size(); ++i)
    if (m_alignedToT[ei][i] == fi)
      return true;

  return false;
}


void PhraseOrientation::IncrementPriorCount(REO_DIR direction, REO_POS orient, float increment)
{
  assert(direction==L2R || direction==R2L);
  if (direction == L2R) {
    m_l2rOrientationPriorCounts[orient] += increment;
  } else if (direction == R2L) {
    m_r2lOrientationPriorCounts[orient] += increment;
  }
}


void PhraseOrientation::WritePriorCounts(std::ostream& out, const REO_MODEL_TYPE modelType)
{
  std::map<std::string,float> l2rOrientationPriorCountsMap;
  std::map<std::string,float> r2lOrientationPriorCountsMap;
  for (int orient=0; orient<=UNKNOWN; ++orient) {
    l2rOrientationPriorCountsMap[GetOrientationString((REO_POS)orient, modelType)] += m_l2rOrientationPriorCounts[orient];
  } 
  for (int orient=0; orient<=UNKNOWN; ++orient) {
    r2lOrientationPriorCountsMap[GetOrientationString((REO_POS)orient, modelType)] += m_r2lOrientationPriorCounts[orient];
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

}  // namespace GHKM
}  // namespace Moses

