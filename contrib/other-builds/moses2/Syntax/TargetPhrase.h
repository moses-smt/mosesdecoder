/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "../TargetPhrase.h"

namespace Moses2
{
namespace Syntax
{

class TargetPhrase : public Moses2::TargetPhrase
{
	  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);
public:
	  static TargetPhrase *CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str);

	  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system, size_t size)
	  :Moses2::TargetPhrase(pool, pt, system, size)
	  {}

protected:
	  Word m_lhs;

};

}

}

