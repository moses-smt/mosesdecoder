/***********************************************************************
  Moses - factored phrase-based language decoder
  Copyright (C) 2009 University of Edinburgh

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/

#include <string.h>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include "InputFileStream.h"
#include "OutputFileStream.h"

using namespace std;

vector< string > splitLine(const char *line)
{
  vector< string > item;
  int start=0;
  int i=0;
  for(; line[i] != '\0'; i++) {
    if (line[i] == ' ' &&
        line[i+1] == '|' &&
        line[i+2] == '|' &&
        line[i+3] == '|' &&
        line[i+4] == ' ') {
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

  string line;
  if (getline(fileP, line)) {
    item = splitLine(line.c_str());
    return false;
  } else {
    return false;
  }
}


int main(int argc, char* argv[])
{
  cerr << "Starting..." << endl;

  char* &fileNameDirect = argv[1];
  Moses::InputFileStream fileDirect(fileNameDirect);


  //fileDirect.open(fileNameDirect);
  if (fileDirect.fail()) {
    cerr << "ERROR: could not open extract file " << fileNameDirect << endl;
    exit(1);
  }
  istream &fileDirectP = fileDirect;

  char* &fileNameConsolidated = argv[2];
  ostream *fileConsolidated;

  if (strcmp(fileNameConsolidated, "-") == 0) {
    fileConsolidated = &cout;
  } else {
    Moses::OutputFileStream *outputFile = new Moses::OutputFileStream();
    bool success = outputFile->Open(fileNameConsolidated);
    if (!success) {
      cerr << "ERROR: could not open file phrase table file "
           << fileNameConsolidated << endl;
      exit(1);
    }
    fileConsolidated = outputFile;
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

    (*fileConsolidated) << itemDirect[0] << " ||| " << itemDirect[1] << " ||| ";

    // output alignment and probabilities
    (*fileConsolidated)	<< itemDirect[2]						// prob direct
                        << " 2.718" // phrase count feature
                        << " ||| " << itemDirect[3];	// alignment

    // counts
    (*fileConsolidated) << "||| 0 " << itemDirect[4]; // indirect
    (*fileConsolidated) << endl;

  }

  fileConsolidated->flush();
  if (fileConsolidated != &cout) {
    delete fileConsolidated;
  }

  cerr << "Finished" << endl;
}

