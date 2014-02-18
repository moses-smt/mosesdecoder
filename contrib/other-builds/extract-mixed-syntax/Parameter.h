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
	,sourceSyntax(false)
	,targetSyntax(false)

	{}
	virtual ~Parameter();

	int maxSpan;
	bool sourceSyntax, targetSyntax;
};

