/*
 * LatticeArc.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <iostream>

class LatticeArc {
public:
	LatticeArc();
	virtual ~LatticeArc();

	virtual bool IsNonTerm() const = 0;
	virtual const std::string &GetString() const = 0;

	virtual int GetStart() const = 0;
	virtual int GetEnd() const = 0;

	virtual int GetLowestAlignment() const = 0;
	virtual int GetHighestAlignment() const = 0;

	virtual void Output(std::ostream &out) const = 0;

};

