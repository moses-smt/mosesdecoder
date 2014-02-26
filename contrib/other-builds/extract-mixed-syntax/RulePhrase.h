/*
 * RulePhrase.h
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#ifndef RULEPHRASE_H_
#define RULEPHRASE_H_

#include <vector>

class RuleSymbol;

class RulePhrase : public std::vector<const RuleSymbol*>
{

};

#endif /* RULEPHRASE_H_ */
