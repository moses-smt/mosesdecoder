/*
 * Factor.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "util/string_piece.hh"
#include "util/string_piece_hash.hh"

class Factor {
public:
	Factor();
	virtual ~Factor();

  size_t hash() const
  {
	 size_t ret = hash_value(m_string);
	 return ret;
  }

  inline bool operator==(const Factor &compare) const {
	return m_string == compare.m_string;
  }

protected:
	StringPiece m_string;

};

