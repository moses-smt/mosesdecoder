#include <stdlib.h>
#include <iostream>
#include "ReorderingConstraint.h"
#include "Sentence.h"
#include "../TypeDef.h"
#include "../legacy/Bitmap.h"

using namespace std;

namespace Moses2
{
//! destructer
ReorderingConstraint::~ReorderingConstraint()
{
  //if (m_wall != NULL) free(m_wall);
  //if (m_localWall != NULL) free(m_localWall);
}

//! allocate memory for reordering walls
void ReorderingConstraint::InitializeWalls(size_t size, int max_distortion)
{
  m_size = size;

  m_wall = m_pool.Allocate<bool>(size);
  m_localWall = m_pool.Allocate<size_t>(size);

  m_max_distortion = max_distortion;

  for (size_t pos = 0 ; pos < m_size ; pos++) {
    m_wall[pos] = false;
    m_localWall[pos] = NOT_A_ZONE;
  }
}

//! has to be called to localized walls
void ReorderingConstraint::FinalizeWalls()
{
  for(size_t z = 0; z < m_zone.size(); z++ ) {
    const size_t startZone = m_zone[z].first;
    const size_t endZone = m_zone[z].second;// note: wall after endZone is not local
    for( size_t pos = startZone; pos < endZone; pos++ ) {
      if (m_wall[ pos ]) {
        m_localWall[ pos ] = z;
        m_wall[ pos ] = false;
        //cerr << "SETTING local wall " << pos << std::endl;
      }
      // enforce that local walls only apply to innermost zone
      else if (m_localWall[ pos ] != NOT_A_ZONE) {
        size_t assigned_z = m_localWall[ pos ];
        if ((m_zone[assigned_z].first < startZone) ||
            (m_zone[assigned_z].second > endZone)) {
          m_localWall[ pos ] = z;
        }
      }
    }
  }
}

//! set value at a particular position
void ReorderingConstraint::SetWall( size_t pos, bool value )
{
  //cerr << "SETTING reordering wall at position " << pos << std::endl;
  UTIL_THROW_IF2(pos >= m_size, "Wall over length of sentence: " << pos << " >= " << m_size);
  m_wall[pos] = value;
  m_active = true;
}

//! set a reordering zone (once entered, need to finish)
void ReorderingConstraint::SetZone( size_t startPos, size_t endPos )
{
  //cerr << "SETTING zone " << startPos << "-" << endPos << std::endl;
  std::pair<size_t,size_t> newZone;
  newZone.first = startPos;
  newZone.second = endPos;
  m_zone.push_back( newZone );
  m_active = true;
}

//! set walls based on "-monotone-at-punctuation" flag
void ReorderingConstraint::SetMonotoneAtPunctuation( const Sentence &sentence )
{
  for( size_t i=0; i<sentence.GetSize(); i++ ) {
    const Word& word = sentence[i];
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

//! check if the current hypothesis extension violates reordering constraints
bool ReorderingConstraint::Check( const Bitmap &bitmap, size_t startPos, size_t endPos ) const
{
  // nothing to be checked, we are done
  if (! IsActive() ) return true;

  //cerr << "Check " << bitmap << " " << startPos << "-" << endPos;

  // check walls
  size_t firstGapPos = bitmap.GetFirstGapPos();
  // filling first gap -> no wall violation possible
  if (firstGapPos != startPos) {
    // if there is a wall before the last word,
    // we created a gap while moving through wall
    // -> violation
    for( size_t pos = firstGapPos; pos < endPos; pos++ ) {
      if( GetWall( pos ) ) {
        //cerr << " hitting wall " << pos << std::endl;
        return false;
      }
    }
  }

  // monotone -> no violation possible
  size_t lastPos = bitmap.GetLastPos();
  if ((lastPos == NOT_FOUND && startPos == 0) || // nothing translated
      (firstGapPos > lastPos &&  // no gaps
       firstGapPos == startPos)) { // translating first empty word
    //cerr << " montone, fine." << std::endl;
    return true;
  }

  // check zones
  for(size_t z = 0; z < m_zone.size(); z++ ) {
    const size_t startZone = m_zone[z].first;
    const size_t endZone = m_zone[z].second;

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
      //cerr << " outside active zone" << std::endl;
      return false;
    }

    // ok, this is what we know now:
    // * the phrase is in the zone (at least partially)
    // * either zone is already active, or it becomes active now


    // check, if we are setting us up for a dead end due to distortion limits

    // size_t distortionLimit = (size_t)StaticData::Instance().GetMaxDistortion();
    size_t distortionLimit = m_max_distortion;
    if (startPos != firstGapPos && endZone-firstGapPos >= distortionLimit) {
      //cerr << " dead end due to distortion limit" << std::endl;
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
        //cerr << " overlap end, but not completing" << std::endl;
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
        //cerr << " local wall violation" << std::endl;
        return false;
      }
    }

    // passed all checks for this zone, on to the next one
  }

  // passed all checks, no violations
  //cerr << " fine." << std::endl;
  return true;
}

std::ostream &ReorderingConstraint::Debug(std::ostream &out, const System &system) const
{
  out << "Zones:";
  for (size_t i = 0; i < m_zone.size(); ++i) {
    const std::pair<size_t,size_t> &zone1 = m_zone[i];
    out << zone1.first << "-" << zone1.second << " ";
  }

  out << "Walls:";
  for (size_t i = 0; i < m_size; ++i) {
    out << m_wall[i];
  }

  out << " Local walls:";
  for (size_t i = 0; i < m_size; ++i) {
    out << m_localWall[i] << " ";
  }

  return out;
}

} // namespace

