/*
 * EnApacheChunker.h
 *
 *  Created on: 28 Feb 2014
 *      Author: hieu
 */

#pragma once

#include <vector>
#include <string>
#include <iostream>

class EnOpenNLPChunker {
public:
	EnOpenNLPChunker(const std::string &openNLPPath);
	virtual ~EnOpenNLPChunker();
	void Process(std::istream &in, std::ostream &out, const std::vector<std::string> &filterList);
protected:
	const std::string m_openNLPPath;

	void Escape(std::string &line);
	void Unescape(std::string &line);

	void MosesReformat(const std::string &line, std::ostream &out, const std::vector<std::string> &filterList);

	bool UseLabel(const std::string &label, const std::vector<std::string> &filterList) const;
};

