/*
 * Parameter.h
 *
 *  Created on: 17 Feb 2014
 *      Author: hieu
 */

#pragma once

class Parameter
{
public:

	Parameter()
	:maxSpan(10)
	,maxNonTerm(2)
	,sourceSyntax(false)
	,targetSyntax(false)
	{}

	virtual ~Parameter();

	int maxSpan;
	int maxNonTerm;

	bool sourceSyntax, targetSyntax;
};

