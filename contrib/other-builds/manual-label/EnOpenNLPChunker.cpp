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
#include "moses/Util.h"

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
			+ m_openNLPPath + "/bin/opennlp POSTagger "
				+ m_openNLPPath + "/models/en-pos-maxent.bin | "
			+ m_openNLPPath + "/bin/opennlp ChunkerME "
				+ m_openNLPPath + "/models/en-chunker.bin > "
			+ outStr;
	//cerr << "Executing:" << cmd << endl;
	int ret = system(cmd.c_str());

	// read result of chunker and output as Moses xml trees
	ifstream outFile(outStr.c_str());
	while (getline(outFile, line)) {
		//cerr << line << endl;
		MosesReformat(line, out);
		out << endl;
	}
	outFile.close();

	// clean up temporary files
	remove(inStr.c_str());
	remove(outStr.c_str());
}

void EnOpenNLPChunker::MosesReformat(const string &line, std::ostream &out)
{
	vector<string> toks;
	Moses::Tokenize(toks, line);
	for (size_t i = 0; i < toks.size(); ++i) {
		const string &tok = toks[i];

		if (tok.substr(0, 1) == "[") {
			string label = tok.substr(1);
			out << "<tree label='" << label << "'>";
		}
		else if (tok.substr(tok.size()-1, 1) == "]") {
			if (tok.size() > 1) {
				string word = tok.substr(0, tok.size()-1);

				vector<string> factors;
				Moses::Tokenize(factors, word, "_");
				assert(factors.size() == 2);
				out << factors[0] << " ";
			}

			out << "</tree> ";
		}
		else {
			vector<string> factors;
			Moses::Tokenize(factors, tok, "_");
			assert(factors.size() == 2);
			out << factors[0] << " ";
		}
	}
}
