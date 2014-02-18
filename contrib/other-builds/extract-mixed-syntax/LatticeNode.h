/*
 * LatticeNode.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>

class LatticeNode {
public:
	LatticeNode();
	virtual ~LatticeNode();

	virtual bool IsNonTerm() const = 0;
	virtual const std::string &GetString() const = 0;

};

