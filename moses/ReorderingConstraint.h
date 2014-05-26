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

#ifndef moses_ReorderingConstraint_h
#define moses_ReorderingConstraint_h

//#include <malloc.h>
#include <limits>
#include <vector>
#include <iostream>
#include <cstring>
#include <cmath>
#include "TypeDef.h"
#include "Word.h"
#include "Phrase.h"

namespace Moses
{

class InputType;
class WordsBitmap;

#define NOT_A_ZONE 999999999
/** A list of zones and walls to limit which reordering can occur
 */
class ReorderingConstraint
{
  friend std::ostream& operator<<(std::ostream& out, const ReorderingConstraint& reorderingConstraint);
protected:
  // const size_t m_size; /**< number of words in sentence */
  size_t m_size; /**< number of words in sentence */
  bool *m_wall;	/**< flag for each word if it is a wall */
  size_t *m_localWall;	/**< flag for each word if it is a local wall */
  std::vector< std::vector< size_t > > m_zone; /** zones that limit reordering */
  bool   m_active; /**< flag indicating, if there are any active constraints */

public:

  //! create ReorderingConstraint of length size and initialise to zero
  ReorderingConstraint() :m_wall(NULL),m_localWall(NULL),m_active(false) {}

  //! destructer
  ~ReorderingConstraint() {
    if (m_wall != NULL) free(m_wall);
    if (m_localWall != NULL) free(m_localWall);
  }

  //! allocate memory for memory for a sentence of a given size
  void InitializeWalls(size_t size);

  //! changes walls in zones into local walls
  void FinalizeWalls();

  //! set value at a particular position
  void SetWall( size_t pos, bool value );

  //! whether a word has been translated at a particular position
  bool GetWall(size_t pos) const {
    return m_wall[pos];
  }

  //! whether a word has been translated at a particular position
  bool GetLocalWall(size_t pos, size_t zone ) const {
    return (m_localWall[pos] == zone);
  }

  //! set a zone
  void SetZone( size_t startPos, size_t endPos );

  //! returns the vector of zones
  std::vector< std::vector< size_t > > & GetZones() {
    return m_zone;
  }

  //! set the reordering walls based on punctuation in the sentence
  void SetMonotoneAtPunctuation( const Phrase & sentence );

  //! check if all constraints are fulfilled -> all find
  bool Check( const WordsBitmap &bitmap, size_t start, size_t end ) const;

  //! checks if reordering constraints will be enforced
  bool IsActive() const {
    return m_active;
  }
};

}
#endif
