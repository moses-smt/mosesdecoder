/*
 *  Permutation.h
 *  met - Minimum Error Training
 *
 *  Created by Alexandra Birch 18 Nov 2009.
 *
 */

#ifndef PERMUTATION_H
#define PERMUTATION_H


#include <limits>
#include <vector>
#include <iostream>
#include <fstream>

#include "Util.h"

namespace MosesTuning
{


class Permutation
{

public:
  //Can be HAMMING_DISTANCE or KENDALLS_DISTANCE
  Permutation(const std::string &alignment = std::string(), const int sourceLength = 0, const int targetLength = 0 );

  ~Permutation() {};

  inline void clear() {
    m_array.clear();
  }
  inline size_t size() {
    return m_array.size();
  }


  void set(const std::string &alignment,const int sourceLength);

  float distance(const Permutation &permCompare, const distanceMetric_t &strategy = HAMMING_DISTANCE) const;

  //Const
  void dump() const;
  size_t getLength() const;
  std::vector<int> getArray() const;
  int getTargetLength() const {
    return m_targetLength;
  }


  //Static
  static std::string convertMosesToStandard(std::string const & alignment);
  static std::vector<int> invert(std::vector<int> const & inVector);
  static bool checkValidPermutation(std::vector<int> const & inVector);

protected:
  std::vector<int> m_array;
  int m_targetLength;
  float calculateHamming(const Permutation & compare) const;
  float calculateKendall(const Permutation & compare) const;

private:
};


}

#endif
