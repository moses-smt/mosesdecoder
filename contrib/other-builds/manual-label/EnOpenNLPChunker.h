/*
 * EnApacheChunker.h
 *
 *  Created on: 28 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <string>
#include <iostream>

class EnOpenNLPChunker {
public:
	EnOpenNLPChunker(const std::string &openNLPPath);
	virtual ~EnOpenNLPChunker();
	void Process(std::istream &in, std::ostream &out);
protected:
	const std::string m_openNLPPath;
};

