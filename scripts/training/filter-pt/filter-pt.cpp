/*
 *  filter-pt.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 07/12/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <map>
#include <set>
#include <string>
#include <iostream>
#include "filter-pt.h"
#include "InputFileStream.h"
#include "SyntaxTree.h"
#include "XmlTree.h"
#include "Lattice.h"
#include "tables-core.h"
#include "Vocab.h"

using namespace std;

int main(int argc, char* argv[]) 
{
  cerr << "Starting..." << endl;
	
	string inputPath	= argv[1];
	string ptPath			= argv[2];

	Lattice lattice;
  Vocab vocab;
  
	// input sentences
	Moses::InputFileStream inputStream(inputPath);

	long i = 0;
	string line;
	
	while(getline(inputStream, line))
	{
		i++;
		cerr << i << " ";
		
		std::set< std::string >				sourceLabelCollection;
		std::map< std::string, int >	sourceTopLabelCollection;

		SyntaxTree sourceTree;
		
		ProcessAndStripXMLTags(line, sourceTree, sourceLabelCollection, sourceTopLabelCollection);
		sourceTree.AddDefaultNonTerms(true);
		
		std::vector<std::string> toks = tokenize( line.c_str() );
		
		lattice.Add(toks, sourceTree, vocab);
	}
	inputStream.Close();
	
	
	// pt
	Moses::InputFileStream ptStream(ptPath);

	i = 0;
	
	while(getline(ptStream, line))
	{
		i++;
		if (i%10000 == 0) cerr << i << " ";

		bool include = lattice.Include(line, vocab);
		if (include)
		{
			cout << line << endl;
		}
	}
	
  cerr << "Finished" << endl;

}