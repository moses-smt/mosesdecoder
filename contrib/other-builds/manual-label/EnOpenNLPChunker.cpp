/*
 * EnApacheChunker.cpp
 *
 *  Created on: 28 Feb 2014
 *      Author: hieu
 */
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include "EnOpenNLPChunker.h"

using namespace std;

EnOpenNLPChunker::EnOpenNLPChunker(const std::string &openNLPPath)
:m_openNLPPath(openNLPPath)
{
	// TODO Auto-generated constructor stub

}

EnOpenNLPChunker::~EnOpenNLPChunker() {
	// TODO Auto-generated destructor stub
}

void EnOpenNLPChunker::Process(std::istream &in, std::ostream &out)
{
	// read all input to a temp file
	char *ptr = tmpnam(NULL);
	string inStr(ptr);
	ofstream inFile(ptr);

	string line;
	while (getline(in, line)) {
		inFile << line << endl;
	}
	inFile.close();

	ptr = tmpnam(NULL);
	string outStr(ptr);

	// execute chunker
	string cmd = "cat " + inStr + " | "
			+ m_openNLPPath + "bin/opennlp POSTagger "
				+ m_openNLPPath + "/models/en-pos-maxent.bin | "
			+ m_openNLPPath + "bin/opennlp ChunkerME "
				+ m_openNLPPath + "/models/en-chunker.bin > "
			+ outStr;
	int ret = system(cmd.c_str());

	// output result
	ifstream outFile(outStr.c_str());
	while (getline(outFile, line)) {
		out << line << endl;
	}
	outFile.close();

}
