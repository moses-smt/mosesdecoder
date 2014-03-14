// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2008 University of Edinburgh

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

#include "ReorderingConstraint.h"
#include "InputType.h"
#include "StaticData.h"
#include "WordsBitmap.h"

namespace Moses
{

//! allocate memory for reordering walls
void ReorderingConstraint::InitializeWalls(size_t size)
{
  m_size = size;
  m_wall      = (bool*)   malloc(sizeof(bool) * size);
  m_localWall = (size_t*) malloc(sizeof(size_t) * size);

  for (size_t pos = 0 ; pos < m_size ; pos++) {
    m_wall[pos] = false;
    m_localWall[pos] = NOT_A_ZONE;
  }
}


//! set value at a particular position
void ReorderingConstraint::SetWall( size_t pos, bool value )
{
  VERBOSE(3,"SETTING reordering wall at position " << pos << std::endl);
  m_wall[pos] = value;
  m_active = true;
}

//! has to be called to localized walls
void ReorderingConstraint::FinalizeWalls()
{
  for(size_t z = 0; z < m_zone.size(); z++ ) {
    const size_t startZone = m_zone[z][0];
    const size_t endZone = m_zone[z][1];// note: wall after endZone is not local
    for( size_t pos = startZone; pos < endZone; pos++ ) {
      if (m_wall[ pos ]) {
        m_localWall[ pos ] = z;
        m_wall[ pos ] = false;
        VERBOSE(3,"SETTING local wall " << pos << std::endl);
      }
      // enforce that local walls only apply to innermost zone
      else if (m_localWall[ pos ] != NOT_A_ZONE) {
        size_t assigned_z = m_localWall[ pos ];
        if ((m_zone[assigned_z][0] < startZone) ||
            (m_zone[assigned_z][1] > endZone)) {
          m_localWall[ pos ] = z;
        }
      }
    }
  }
}

//! set walls based on "-monotone-at-punctuation" flag
void ReorderingConstraint::SetMonotoneAtPunctuation( const Phrase &sentence )
{
  for( size_t i=0; i<sentence.GetSize(); i++ ) {
    const Word& word = sentence.GetWord(i);
    if (word[0]->GetString() == "," ||
        word[0]->GetString() == "." ||
        word[0]->GetString() == "!" ||
        word[0]->GetString() == "?" ||
        word[0]->GetString() == ":" ||
        word[0]->GetString() == ";" ||
        word[0]->GetString() == "\"") {
      // set wall before and after punc, but not at sentence start, end
      if (i>0 && i<m_size-1) SetWall( i, true );
      if (i>1)               SetWall( i-1, true );
    }
  }
}

//! set a reordering zone (once entered, need to finish)
void ReorderingConstraint::SetZone( size_t startPos, size_t endPos )
{
  VERBOSE(3,"SETTING zone " << startPos << "-" << endPos << std::endl);
  std::vector< size_t > newZone;
  newZone.push_back( startPos );
  newZone.push_back( endPos );
  m_zone.push_back( newZone );
  m_active = true;
}

//! check if the current hypothesis extension violates reordering constraints
bool ReorderingConstraint::Check( const WordsBitmap &bitmap, size_t startPos, size_t endPos ) const
{
  // nothing to be checked, we are done
  if (! IsActive() ) return true;

  VERBOSE(3,"Check " << bitmap << " " << startPos << "-" << endPos);

  // check walls
  size_t firstGapPos = bitmap.GetFirstGapPos();
  // filling first gap -> no wall violation possible
  if (firstGapPos != startPos) {
    // if there is a wall before the last word,
    // we created a gap while moving through wall
    // -> violation
    for( size_t pos = firstGapPos; pos < endPos; pos++ ) {
      if( GetWall( pos ) ) {
        VERBOSE(3," hitting wall " << pos << std::endl);
        return false;
      }
    }
  }

  // monotone -> no violation possible
  size_t lastPos = bitmap.GetLastPos();
  if ((lastPos == NOT_FOUND && startPos == 0) || // nothing translated
      (firstGapPos > lastPos &&  // no gaps
       firstGapPos == startPos)) { // translating first empty word
    VERBOSE(3," montone, fine." << std::endl);
    return true;
  }

  // check zones
  for(size_t z = 0; z < m_zone.size(); z++ ) {
    const size_t startZone = m_zone[z][0];
    const size_t endZone = m_zone[z][1];

    // fine, if translation has not reached zone yet and phrase outside zone
    if (lastPos < startZone && ( endPos < startZone || startPos > endZone ) ) {
      continue;
    }

    // already completely translated zone, no violations possible
    if (firstGapPos > endZone) {
      continue;
    }

    // some words are translated beyond the start
    // let's look closer if some are in the zone
    size_t numWordsInZoneTranslated = 0;
    if (lastPos >= startZone) {
      for(size_t pos = startZone; pos <= endZone; pos++ ) {
        if( bitmap.GetValue( pos ) ) {
          numWordsInZoneTranslated++;
        }
      }
    }

    // all words in zone translated, no violation possible
    if (numWordsInZoneTranslated == endZone-startZone+1) {
      continue;
    }

    // flag if this is an active zone
    bool activeZone = (numWordsInZoneTranslated > 0);

    // fine, if zone completely untranslated and phrase outside zone
    if (!activeZone && ( endPos < startZone || startPos > endZone ) ) {
      continue;
    }

    // violation, if phrase completely outside active zone
    if (activeZone && ( endPos < startZone || startPos > endZone ) ) {
      VERBOSE(3," outside active zone" << std::endl);
      return false;
    }

    // ok, this is what we know now:
    // * the phrase is in the zone (at least partially)
    // * either zone is already active, or it becomes active now


    // check, if we are setting us up for a dead end due to distortion limits
    size_t distortionLimit = (size_t)StaticData::Instance().GetMaxDistortion();
    if (startPos != firstGapPos && endZone-firstGapPos >= distortionLimit) {
      VERBOSE(3," dead end due to distortion limit" << std::endl);
      return false;
    }

    // let us check on phrases that are partially outside

    // phrase overlaps at the beginning, always ok
    if (startPos <= startZone) {
      continue;
    }

    // phrase goes beyond end, has to fill zone completely
    if (endPos > endZone) {
      if (endZone-startPos+1 < // num. words filled in by phrase
          endZone-startZone+1-numWordsInZoneTranslated) { // num. untranslated
        VERBOSE(3," overlap end, but not completing" << std::endl);
        return false;
      } else {
        continue;
      }
    }

    // now we are down to phrases that are completely inside the zone
    // we have to check local walls
    bool seenUntranslatedBeforeStartPos = false;
    for(size_t pos = startZone; pos < endZone && pos < endPos; pos++ ) {
      // be careful when there is a gap before phrase
      if( !bitmap.GetValue( pos ) // untranslated word
          && pos < startPos ) {   // before startPos
        seenUntranslatedBeforeStartPos = true;
      }
      if( seenUntranslatedBeforeStartPos && GetLocalWall( pos, z ) ) {
        VERBOSE(3," local wall violation" << std::endl);
        return false;
      }
    }

    // passed all checks for this zone, on to the next one
  }

  // passed all checks, no violations
  VERBOSE(3," fine." << std::endl);
  return true;
}

}
