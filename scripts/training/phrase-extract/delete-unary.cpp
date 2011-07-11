/*
 *  delete-unary.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 29/11/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "delete-unary.h"
#include "SafeGetline.h"

#define LINE_MAX_LENGTH 100000

using namespace std;

// speeded up version of above
void Tokenize(std::vector<std::string> &output
										 , const std::string& str
										 , const std::string& delimiters = " \t")
{
	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
	
	while (std::string::npos != pos || std::string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		output.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

bool IsNonTerm(const string &word)
{
	size_t len = word.size();
	if (word.substr(0, 1) == "[" && word.substr(len - 1, 1) == "]")
	{
		return true;
	}
	else 
	{
		return false;
	}

}

int main(int argc, char* argv[]) 
{
	long i = 0;
	string line;
	
	while(true) {
		getline(cin, line);

		if (cin.eof()) break;
		//cerr << ++i << " " << flush;
		
		vector<string> toks, sourceWords;
		Tokenize(toks, line, "|");
		assert(toks.size() == 4);

		string &source = toks[0];
		
		bool deleteWord = false;
		Tokenize(sourceWords, source, " ");
		if (sourceWords.size() == 2)
		{
			if (IsNonTerm(sourceWords[0]) && IsNonTerm(sourceWords[1]))
			{
				deleteWord = true;
			}
		}
		
		if (!deleteWord)
		{
			cout << line << endl;
		}
	}
}