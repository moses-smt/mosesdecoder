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

#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>

#include "tables-core.h"
#include "SafeGetline.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"

#define LINE_MAX_LENGTH 10000

using namespace std;

bool hierarchicalFlag = false;
bool onlyDirectFlag = false;
bool phraseCountFlag = false;
bool lowCountFlag = false;
bool goodTuringFlag = false;
bool kneserNeyFlag = false;
bool logProbFlag = false;
inline float maybeLogProb( float a )
{
  return logProbFlag ? log(a) : a;
}

char line[LINE_MAX_LENGTH];
void processFiles( char*, char*, char*, char* );
void loadCountOfCounts( char* );
void breakdownCoreAndSparse( string combined, string &core, string &sparse );
bool getLine( istream &fileP, vector< string > &item );
vector< string > splitLine();
vector< int > countBin;
bool sparseCountBinFeatureFlag = false;

int main(int argc, char* argv[])
{
  cerr << "Consolidate v2.0 written by Philipp Koehn\n"
       << "consolidating direct and indirect rule tables\n";

  if (argc < 4) {
    cerr << "syntax: consolidate phrase-table.direct phrase-table.indirect phrase-table.consolidated [--Hierarchical] [--OnlyDirect] \n";
    exit(1);
  }
  char* &fileNameDirect = argv[1];
  char* &fileNameIndirect = argv[2];
  char* &fileNameConsolidated = argv[3];
  char* fileNameCountOfCounts;

  for(int i=4; i<argc; i++) {
    if (strcmp(argv[i],"--Hierarchical") == 0) {
      hierarchicalFlag = true;
      cerr << "processing hierarchical rules\n";
    } else if (strcmp(argv[i],"--OnlyDirect") == 0) {
      onlyDirectFlag = true;
      cerr << "only including direct translation scores p(e|f)\n";
    } else if (strcmp(argv[i],"--PhraseCount") == 0) {
      phraseCountFlag = true;
      cerr << "including the phrase count feature\n";
    } else if (strcmp(argv[i],"--GoodTuring") == 0) {
      goodTuringFlag = true;
      if (i+1==argc) {
        cerr << "ERROR: specify count of count files for Good Turing discounting!\n";
        exit(1);
      }
      fileNameCountOfCounts = argv[++i];
      cerr << "adjusting phrase translation probabilities with Good Turing discounting\n";
    } else if (strcmp(argv[i],"--KneserNey") == 0) {
      kneserNeyFlag = true;
      if (i+1==argc) {
        cerr << "ERROR: specify count of count files for Kneser Ney discounting!\n";
        exit(1);
      }
      fileNameCountOfCounts = argv[++i];
      cerr << "adjusting phrase translation probabilities with Kneser Ney discounting\n";
    } else if (strcmp(argv[i],"--LowCountFeature") == 0) {
      lowCountFlag = true;
      cerr << "including the low count feature\n";
    } else if (strcmp(argv[i],"--CountBinFeature") == 0 ||
               strcmp(argv[i],"--SparseCountBinFeature") == 0) {
      if (strcmp(argv[i],"--SparseCountBinFeature") == 0)
        sparseCountBinFeatureFlag = true;
      cerr << "include "<< (sparseCountBinFeatureFlag ? "sparse " : "") << "count bin feature:";
      int prev = 0;
      while(i+1<argc && argv[i+1][0]>='0' && argv[i+1][0]<='9') {
        int binCount = atoi(argv[++i]);
        countBin.push_back( binCount );
        if (prev+1 == binCount) {
          cerr << " " << binCount;
        } else {
          cerr << " " << (prev+1) << "-" << binCount;
        }
        prev = binCount;
      }
      cerr << " " << (prev+1) << "+\n";
    } else if (strcmp(argv[i],"--LogProb") == 0) {
      logProbFlag = true;
      cerr << "using log-probabilities\n";
    } else {
      cerr << "ERROR: unknown option " << argv[i] << endl;
      exit(1);
    }
  }

  processFiles( fileNameDirect, fileNameIndirect, fileNameConsolidated, fileNameCountOfCounts );
}

vector< float > countOfCounts;
vector< float > goodTuringDiscount;
float kneserNey_D1, kneserNey_D2, kneserNey_D3, totalCount = -1;
void loadCountOfCounts( char* fileNameCountOfCounts )
{
  Moses::InputFileStream fileCountOfCounts(fileNameCountOfCounts);
  if (fileCountOfCounts.fail()) {
    cerr << "ERROR: could not open count of counts file " << fileNameCountOfCounts << endl;
    exit(1);
  }
  istream &fileP = fileCountOfCounts;

  countOfCounts.push_back(0.0);
  while(1) {
    if (fileP.eof()) break;
    SAFE_GETLINE((fileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
    if (fileP.eof()) break;
    if (totalCount < 0)
      totalCount = atof(line); // total number of distinct phrase pairs
    else
      countOfCounts.push_back( atof(line) );
  }
  fileCountOfCounts.Close();

  // compute Good Turing discounts
  if (goodTuringFlag) {
    goodTuringDiscount.push_back(0.01); // floor value
    for( size_t i=1; i<countOfCounts.size()-1; i++ ) {
      goodTuringDiscount.push_back(((float)i+1)/(float)i*((countOfCounts[i+1]+0.1) / ((float)countOfCounts[i]+0.1)));
      if (goodTuringDiscount[i]>1)
        goodTuringDiscount[i] = 1;
      if (goodTuringDiscount[i]<goodTuringDiscount[i-1])
        goodTuringDiscount[i] = goodTuringDiscount[i-1];
    }
  }

  // compute Kneser Ney co-efficients [Chen&Goodman, 1998]
  float Y = countOfCounts[1] / (countOfCounts[1] + 2*countOfCounts[2]);
  kneserNey_D1 = 1 - 2*Y * countOfCounts[2] / countOfCounts[1];
  kneserNey_D2 = 2 - 3*Y * countOfCounts[3] / countOfCounts[2];
  kneserNey_D3 = 3 - 4*Y * countOfCounts[4] / countOfCounts[3];
  // sanity constraints
  if (kneserNey_D1 > 0.9) kneserNey_D1 = 0.9;
  if (kneserNey_D2 > 1.9) kneserNey_D2 = 1.9;
  if (kneserNey_D3 > 2.9) kneserNey_D3 = 2.9;
}

void processFiles( char* fileNameDirect, char* fileNameIndirect, char* fileNameConsolidated, char* fileNameCountOfCounts )
{
  if (goodTuringFlag || kneserNeyFlag)
    loadCountOfCounts( fileNameCountOfCounts );

  // open input files
  Moses::InputFileStream fileDirect(fileNameDirect);
  Moses::InputFileStream fileIndirect(fileNameIndirect);

  if (fileDirect.fail()) {
    cerr << "ERROR: could not open phrase table file " << fileNameDirect << endl;
    exit(1);
  }
  istream &fileDirectP = fileDirect;

  if (fileIndirect.fail()) {
    cerr << "ERROR: could not open phrase table file " << fileNameIndirect << endl;
    exit(1);
  }
  istream &fileIndirectP = fileIndirect;

  // open output file: consolidated phrase table
  Moses::OutputFileStream fileConsolidated;
  bool success = fileConsolidated.Open(fileNameConsolidated);
  if (!success) {
    cerr << "ERROR: could not open output file " << fileNameConsolidated << endl;
    exit(1);
  }

  // loop through all extracted phrase translations
  int i=0;
  while(true) {
    i++;
    if (i%100000 == 0) cerr << "." << flush;

    vector< string > itemDirect, itemIndirect;
    if (! getLine(fileIndirectP,itemIndirect) ||
        ! getLine(fileDirectP,  itemDirect  ))
      break;

    // direct: target source alignment probabilities
    // indirect: source target probabilities

    // consistency checks
    if (itemDirect[0].compare( itemIndirect[0] ) != 0) {
      cerr << "ERROR: target phrase does not match in line " << i << ": '"
           << itemDirect[0] << "' != '" << itemIndirect[0] << "'" << endl;
      exit(1);
    }

    if (itemDirect[1].compare( itemIndirect[1] ) != 0) {
      cerr << "ERROR: source phrase does not match in line " << i << ": '"
           << itemDirect[1] << "' != '" << itemIndirect[1] << "'" << endl;
      exit(1);
    }

    // output hierarchical phrase pair (with separated labels)
    fileConsolidated << itemDirect[0] << " ||| " << itemDirect[1] << " |||";

    // SCORES ...
    string directScores, directSparseScores, indirectScores, indirectSparseScores;
    breakdownCoreAndSparse( itemDirect[2], directScores, directSparseScores );
    breakdownCoreAndSparse( itemIndirect[2], indirectScores, indirectSparseScores );

    vector<string> directCounts = tokenize(itemDirect[4].c_str());
    vector<string> indirectCounts = tokenize(itemIndirect[4].c_str());
    float countF = atof(directCounts[0].c_str());
    float countE = atof(indirectCounts[0].c_str());
    float countEF = atof(indirectCounts[1].c_str());
    float n1_F, n1_E;
    if (kneserNeyFlag) {
      n1_F = atof(directCounts[2].c_str());
      n1_E = atof(indirectCounts[2].c_str());
    }

    // Good Turing discounting
    float adjustedCountEF = countEF;
    if (goodTuringFlag && countEF+0.99999 < goodTuringDiscount.size()-1)
      adjustedCountEF *= goodTuringDiscount[(int)(countEF+0.99998)];
    float adjustedCountEF_indirect = adjustedCountEF;

    // Kneser Ney discounting [Foster et al, 2006]
    if (kneserNeyFlag) {
      float D = kneserNey_D3;
      if (countEF < 2) D = kneserNey_D1;
      else if (countEF < 3) D = kneserNey_D2;
      if (D > countEF) D = countEF - 0.01; // sanity constraint

      float p_b_E = n1_E / totalCount; // target phrase prob based on distinct
      float alpha_F = D * n1_F / countF; // available mass
      adjustedCountEF = countEF - D + countF * alpha_F * p_b_E;

      // for indirect
      float p_b_F = n1_F / totalCount; // target phrase prob based on distinct
      float alpha_E = D * n1_E / countE; // available mass
      adjustedCountEF_indirect = countEF - D + countE * alpha_E * p_b_F;
    }

    // prob indirect
    if (!onlyDirectFlag) {
      fileConsolidated << " " << maybeLogProb(adjustedCountEF_indirect/countE);
      fileConsolidated << " " << indirectScores;
    }

    // prob direct
    fileConsolidated << " " << maybeLogProb(adjustedCountEF/countF);
    fileConsolidated << " " << directScores;

    // phrase count feature
    if (phraseCountFlag) {
      fileConsolidated << " " << maybeLogProb(2.718);
    }

    // low count feature
    if (lowCountFlag) {
      fileConsolidated << " " << maybeLogProb(exp(-1.0/countEF));
    }

    // count bin feature (as a core feature)
    if (countBin.size()>0 && !sparseCountBinFeatureFlag) {
      bool foundBin = false;
      for(size_t i=0; i < countBin.size(); i++) {
        if (!foundBin && countEF <= countBin[i]) {
          fileConsolidated << " " << maybeLogProb(2.718);
          foundBin = true;
        } else {
          fileConsolidated << " " << maybeLogProb(1);
        }
      }
      fileConsolidated << " " << maybeLogProb( foundBin ? 1 : 2.718 );
    }

    // alignment
    fileConsolidated << " ||| " << itemDirect[3];

    // counts, for debugging
    fileConsolidated << "||| " << countE << " " << countF << " " << countEF;

    // count bin feature (as a sparse feature)
    fileConsolidated << " |||";
    if (directSparseScores.compare("") != 0)
      fileConsolidated << " " << directSparseScores;
    if (indirectSparseScores.compare("") != 0)
      fileConsolidated << " " << indirectSparseScores;
    if (sparseCountBinFeatureFlag) {
      bool foundBin = false;
      for(size_t i=0; i < countBin.size(); i++) {
        if (!foundBin && countEF <= countBin[i]) {
          fileConsolidated << " cb_";
          if (i == 0 && countBin[i] > 1)
            fileConsolidated << "1_";
          else if (i > 0 && countBin[i-1]+1 < countBin[i])
            fileConsolidated << (countBin[i-1]+1) << "_";
          fileConsolidated << countBin[i] << " 1";
          foundBin = true;
        }
      }
      if (!foundBin) {
        fileConsolidated << " cb_max 1";
      }
    }

    // arbitrary key-value pairs
    if (itemDirect.size() >= 6) {
      fileConsolidated << " ||| " << itemDirect[5];
    }

    fileConsolidated << endl;
  }
  fileDirect.Close();
  fileIndirect.Close();
  fileConsolidated.Close();
}

void breakdownCoreAndSparse( string combined, string &core, string &sparse )
{
  core = "";
  sparse = "";
  vector<string> score = tokenize( combined.c_str() );
  for(size_t i=0; i<score.size(); i++) {
    if ((score[i][0] >= '0' && score[i][0] <= '9') || i+1 == score.size())
      core += " " + score[i];
    else {
      sparse += " " + score[i];
      sparse += " " + score[++i];
    }
  }
  if (core.size() > 0 ) core = core.substr(1);
  if (sparse.size() > 0 ) sparse = sparse.substr(1);
}

bool getLine( istream &fileP, vector< string > &item )
{
  if (fileP.eof())
    return false;

  SAFE_GETLINE((fileP), line, LINE_MAX_LENGTH, '\n', __FILE__);
  if (fileP.eof())
    return false;

  item = splitLine();

  return true;
}

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
