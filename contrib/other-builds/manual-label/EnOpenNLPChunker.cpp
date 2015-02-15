/*
 * EnApacheChunker.cpp
 *
 *  Created on: 28 Feb 2014
 *      Author: hieu
 */
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <fstream>
#include <boost/algorithm/string/predicate.hpp>
#include "EnOpenNLPChunker.h"
#include "moses/Util.h"

using namespace std;
using namespace boost::algorithm;

EnOpenNLPChunker::EnOpenNLPChunker(const std::string &openNLPPath)
:m_openNLPPath(openNLPPath)
{
	// TODO Auto-generated constructor stub

}

EnOpenNLPChunker::~EnOpenNLPChunker() {
	// TODO Auto-generated destructor stub
}

void EnOpenNLPChunker::Process(std::istream &in, std::ostream &out, const vector<string> &filterList)
{
	// read all input to a temp file
	char *ptr = tmpnam(NULL);
	string inStr(ptr);
	ofstream inFile(ptr);

	string line;
	while (getline(in, line)) {
		Unescape(line);
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
	//g << "Executing:" << cmd << endl;
	int ret = system(cmd.c_str());

	// read result of chunker and output as Moses xml trees
	ifstream outFile(outStr.c_str());

	size_t lineNum = 0;
	while (getline(outFile, line)) {
		//cerr << line << endl;
		MosesReformat(line, out, filterList);
		out << endl;
		++lineNum;
	}
	outFile.close();

	// clean up temporary files
	remove(inStr.c_str());
	remove(outStr.c_str());
}

void EnOpenNLPChunker::MosesReformat(const string &line, std::ostream &out, const vector<string> &filterList)
{
	//cerr << "REFORMATING:" << line << endl;
	bool inLabel = false;
	vector<string> toks;
	Moses::Tokenize(toks, line);
	for (size_t i = 0; i < toks.size(); ++i) {
		const string &tok = toks[i];

		if (tok.substr(0, 1) == "[" && tok.substr(1,1) != "_") {
			// start of chunk
			string label = tok.substr(1);
			if (UseLabel(label, filterList)) {
				out << "<tree label=\"" << label << "\">";
				inLabel = true;
			}
		}
		else if (ends_with(tok, "]")) {
			// end of chunk
			if (tok.size() > 1) {
				if (tok.substr(1,1) == "_") {
					// just a word that happens to be ]
					vector<string> factors;
					Moses::Tokenize(factors, tok, "_");
					assert(factors.size() == 2);

					Escape(factors[0]);
					out << factors[0] << " ";
				}
				else {
					// a word and end of tree
					string word = tok.substr(0, tok.size()-1);

					vector<string> factors;
					Moses::Tokenize(factors, word, "_");
					assert(factors.size() == 2);

					Escape(factors[0]);
					out << factors[0] << " ";
				}

				if (inLabel) {
					out << "</tree> ";
					inLabel = false;
				}
			}
			else {
				if (inLabel) {
					out << "</tree> ";
					inLabel = false;
				}
			}

		}
		else {
			// lexical item
			vector<string> factors;
			Moses::Tokenize(factors, tok, "_");
			if (factors.size() == 2) {
				Escape(factors[0]);
				out << factors[0] << " ";
			}
			else if (factors.size() == 1) {
				// word is _
				assert(tok.substr(0, 2) == "__");
				out << "_ ";
			}
			else {
				throw "Unknown format:" + tok;
			}
		}
	}
}

std::string
replaceAll( std::string const& original,
            std::string const& before,
            std::string const& after )
{
    std::string retval;
    std::string::const_iterator end     = original.end();
    std::string::const_iterator current = original.begin();
    std::string::const_iterator next    =
            std::search( current, end, before.begin(), before.end() );
    while ( next != end ) {
        retval.append( current, next );
        retval.append( after );
        current = next + before.size();
        next = std::search( current, end, before.begin(), before.end() );
    }
    retval.append( current, next );
    return retval;
}

void EnOpenNLPChunker::Escape(string &line)
{
	line = replaceAll(line, "&", "&amp;");
	line = replaceAll(line, "|", "&#124;");
	line = replaceAll(line, "<", "&lt;");
	line = replaceAll(line, ">", "&gt;");
	line = replaceAll(line, "'", "&apos;");
	line = replaceAll(line, "\"", "&quot;");
	line = replaceAll(line, "[", "&#91;");
	line = replaceAll(line, "]", "&#93;");
}

void EnOpenNLPChunker::Unescape(string &line)
{
	line = replaceAll(line, "&#124;", "|");
	line = replaceAll(line, "&lt;", "<");
	line = replaceAll(line, "&gt;", ">");
	line = replaceAll(line, "&quot;", "\"");
	line = replaceAll(line, "&apos;", "'");
	line = replaceAll(line, "&#91;", "[");
	line = replaceAll(line, "&#93;", "]");
	line = replaceAll(line, "&amp;", "&");
}

bool EnOpenNLPChunker::UseLabel(const std::string &label, const std::vector<std::string> &filterList) const
{
	if (filterList.size() == 0) {
		return true;
	}

	for (size_t i = 0; i < filterList.size(); ++i) {
		if (label == filterList[i]) {
			return true;
		}
	}
	return false;
}
