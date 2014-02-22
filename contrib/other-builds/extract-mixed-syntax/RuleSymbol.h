/*
 * RuleSymbol.h
 *
 *  Created on: 21 Feb 2014
 *      Author: hieu
 */

#ifndef RULESYMBOL_H_
#define RULESYMBOL_H_

#include <iostream>
#include <string>

class RuleSymbol {
public:
	RuleSymbol();
	virtual ~RuleSymbol();

	virtual std::string Debug() const = 0;

};

#endif /* RULESYMBOL_H_ */
