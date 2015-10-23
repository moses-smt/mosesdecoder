/*
 * Vocab.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef VOCAB_H_
#define VOCAB_H_

#include <boost/unordered_set.hpp>
#include "util/string_piece.hh"
#include "moses/Util.h"
#include "Factor.h"

class Vocab {
public:
  Vocab();
  virtual ~Vocab();

  const Factor *AddFactor(const StringPiece &string);

protected:
  typedef boost::unordered_set<Factor, Moses::UnorderedComparer<Factor>, Moses::UnorderedComparer<Factor> > Set;
  Set m_set;


};

#endif /* VOCAB_H_ */
