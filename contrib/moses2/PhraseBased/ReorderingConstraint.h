#pragma once
#include <iostream>
#include <vector>
#include "../Vector.h"

namespace Moses2
{
class System;
class Sentence;
class Bitmap;
class MemPool;

#define NOT_A_ZONE 999999999

class ReorderingConstraint
{
protected:
  // const size_t m_size; /**< number of words in sentence */
  size_t m_size; /**< number of words in sentence */
  bool *m_wall; /**< flag for each word if it is a wall */
  //size_t *m_wall; /**< flag for each word if it is a wall */
  size_t *m_localWall;  /**< flag for each word if it is a local wall */
  Vector< std::pair<size_t,size_t> > m_zone; /** zones that limit reordering */
  bool   m_active; /**< flag indicating, if there are any active constraints */
  int m_max_distortion;
  MemPool &m_pool;

  ReorderingConstraint(const ReorderingConstraint &); // do not implement

public:

  //! create ReorderingConstraint of length size and initialise to zero
  ReorderingConstraint(MemPool &pool)
    : m_wall(NULL)
    , m_localWall(NULL)
    , m_active(false)
    , m_pool(pool)
    , m_zone(pool)
  {}

  //! destructer
  ~ReorderingConstraint();

  //! allocate memory for memory for a sentence of a given size
  void InitializeWalls(size_t size, int max_distortion);

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
  Vector< std::pair< size_t,size_t> > & GetZones() {
    return m_zone;
  }

  //! set the reordering walls based on punctuation in the sentence
  void SetMonotoneAtPunctuation( const Sentence & sentence );

  //! check if all constraints are fulfilled -> all find
  bool Check( const Bitmap &bitmap, size_t start, size_t end ) const;

  //! checks if reordering constraints will be enforced
  bool IsActive() const {
    return m_active;
  }

  std::ostream &Debug(std::ostream &out, const System &system) const;

};


}

