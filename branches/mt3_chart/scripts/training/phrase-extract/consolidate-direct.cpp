/*
 *  AddConst.cpp
 *  extract
 *
 *  Created by Hieu Hoang on 19/10/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <fstream>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

#define SAFE_GETLINE(_IS, _LINE, _SIZE, _DELIM) { \
_IS.getline(_LINE, _SIZE, _DELIM);						\
if(_IS.fail() && !_IS.bad() && !_IS.eof()) _IS.clear();	\
if (_IS.gcount() == _SIZE-1) {								\
cerr << "Line too long! Buffer overflow. Delete lines >="	\
<< _SIZE << " chars or raise LINE_MAX_LENGTH in phrase-extract/scrore.cpp" \
<< endl;																\
exit(1);																		\
}																							\
}
#define LINE_MAX_LENGTH 10000

char line[LINE_MAX_LENGTH];


vector< string > splitLine() 
{
  vector< string > item;
  int start=0;
	int i=0;
  for(; line[i] != '\0'; i++) {
		if (line[i] == ' ' &&
				line[i+1] == '|' &&
				line[i+2] == '|' &&
				line[i+3] == '|' &&
				line[i+4] == ' ')
		{
			if (start > i) start = i; // empty item
			item.push_back( string( line+start, i-start ) );
			start = i+5;
			i += 3;
		}
	}
	item.push_back( string( line+start, i-start ) );
	
  return item;
}

bool getLine( istream &fileP, vector< string > &item ) 
{
	if (fileP.eof()) 
		return false;
	
	SAFE_GETLINE((fileP), line, LINE_MAX_LENGTH, '\n');
	if (fileP.eof()) 
		return false;
	
	//cerr << line << endl;
	
	item = splitLine();
	
	/*
	 for (size_t i = 0; i < item.size(); i++)
	 cerr << item[i] << "<<";
	 cerr << endl;
	 */
	
	return true;
} 


int main(int argc, char* argv[]) 
{
  cerr << "Starting..." << endl;

	bool hierarchicalFlag = true;
	
	char* &fileNameDirect = argv[1];
	ifstream fileDirect;
	
	fileDirect.open(fileNameDirect);
	if (fileDirect.fail()) {
    cerr << "ERROR: could not open extract file " << fileNameDirect << endl;
    exit(1);
  }
	istream &fileDirectP = fileDirect;

  char* &fileNameConsolidated = argv[2];
	ofstream fileConsolidated;
	fileConsolidated.open(fileNameConsolidated);
  if (fileConsolidated.fail()) 
	{
    cerr << "ERROR: could not open output file " << fileNameConsolidated << endl;
    exit(1);
  }
	
	int i=0;
  while(true) {
		i++;
    if (i%1000 == 0) cerr << "." << flush;
    if (i%10000 == 0) cerr << ":" << flush;
    if (i%100000 == 0) cerr << "!" << flush;
		
		vector< string > itemDirect;
		if (! getLine(fileDirectP,  itemDirect  ))
			break;
			
		
		// output hierarchical phrase pair (with separated labels)
		if (hierarchicalFlag) {
			size_t spaceSource = itemDirect[0].find(" ");
			size_t spaceTarget = itemDirect[1].find(" ");
			if (spaceSource == string::npos) {
				cerr << "ERROR: expected source as 'label words' in line " 
				<< i << ": '" << itemDirect[1] << "'" << endl;
				exit(1);
			}
			if (spaceTarget == string::npos) {
				cerr << "ERROR: expected target as 'label words' in line " 
				<< i << ": '" << itemDirect[0] << "'" << endl;
				exit(1);
			}
			fileConsolidated 
			<< itemDirect[0].substr(0,spaceSource)            // label source
			<< " " << itemDirect[1].substr(0,spaceTarget)     // label target
			<< " ||| " << itemDirect[0].substr(spaceSource+1) // source
			<< " ||| " << itemDirect[1].substr(spaceTarget+1) // target
			<< " ||| ";
		}
		// output regular phrase pair
		else {
			fileConsolidated << itemDirect[0] << " ||| " << itemDirect[1] << " ||| ";
		}
		
		// output alignment and probabilities
		if (hierarchicalFlag) 
			fileConsolidated << itemDirect[2]   << " ||| " // alignment
											<< " " << itemDirect[3]; // prob direct

		fileConsolidated << "  2.718"; // phrase count feature
		
		// counts
		fileConsolidated << " ||| " << itemDirect[4] << " 0"; // indirect
		fileConsolidated << endl;
		
	}
	
  cerr << "Finished" << endl;
}

