/*
 * Vocab.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#ifndef VOCAB_H_
#define VOCAB_H_

#include <boost/unordered_set.hpp>
//#include "util/string_piece.hh"
//#include "moses/Util.h"

class Vocab {
public:
  static Vocab& Instance() {
	return s_instance;
  }

  Vocab();
  virtual ~Vocab();

  //const Factor *AddFactor(const StringPiece &factorString);

protected:
  static Vocab s_instance;

  //typedef boost::unordered_set<Factor*, Moses::UnorderedComparer<Factor>, Moses::UnorderedComparer<Factor>> Set;
  //Set m_set;


};

#endif /* VOCAB_H_ */
