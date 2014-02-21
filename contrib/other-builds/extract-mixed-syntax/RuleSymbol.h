/*
 * RuleSymbol.h
 *
 *  Created on: 21 Feb 2014
 *      Author: hieu
 */

#ifndef RULESYMBOL_H_
#define RULESYMBOL_H_

#include <iostream>

class RuleSymbol {
public:
	RuleSymbol();
	virtual ~RuleSymbol();

	virtual void Debug(std::ostream &out) const = 0;

};

#endif /* RULESYMBOL_H_ */
