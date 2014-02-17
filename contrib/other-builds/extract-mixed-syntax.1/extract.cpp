// $Id: extract.cpp 2828 2010-02-01 16:07:58Z hieuhoang1972 $
// vim:tabstop=2

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
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include "extract.h"
#include "InputFileStream.h"
#include "OutputFileStream.h"
#include "Lattice.h"

#ifdef WIN32
// Include Visual Leak Detector
#include <vld.h>
#endif

using namespace std;

void writeGlueGrammar(const string &, Global &options, set< string > &targetLabelCollection, map< string, int > &targetTopLabelCollection);

int main(int argc, char* argv[]) 
{
  cerr << "Extract v2.0, written by Philipp Koehn\n"
       << "rule extraction from an aligned parallel corpus\n";
  //time_t starttime = time(NULL);
	
	Global *global = new Global();
	g_global = global;
	int sentenceOffset = 0;
		
	if (argc < 5) {
		cerr << "syntax: extract-mixed-syntax corpus.target corpus.source corpus.align extract "
		     << " [ --Hierarchical | --Orientation"
				 << " | --GlueGrammar FILE | --UnknownWordLabel FILE"
				 << " | --OnlyDirect"
					
					<< " | --MinHoleSpanSourceDefault[" << global->minHoleSpanSourceDefault << "]"
					<< " | --MaxHoleSpanSourceDefault[" << global->maxHoleSpanSourceDefault << "]"
					<< " | --MinHoleSpanSourceSyntax[" << global->minHoleSpanSourceSyntax << "]"
					<< " | --MaxHoleSpanSourceSyntax[" << global->maxHoleSpanSourceSyntax << "]"

				<< " | --MaxSymbolsSource[" << global->maxSymbolsSource<< "]"
		     << " | --MaxSymbolsTarget[" << global->maxSymbolsTarget<< "]"
				 << " | --MaxNonTerm[" << global->maxNonTerm << "]"
		     << " | --SourceSyntax | --TargetSyntax" 
				<<	" | --UppermostOnly[" << g_global->uppermostOnly << "]"
				<< endl;
		exit(1);
	}
  char* &fileNameT = argv[1];
  char* &fileNameS = argv[2];
  char* &fileNameA = argv[3];
	string fileNameGlueGrammar;
 	string fileNameUnknownWordLabel;
	string fileNameExtract = string(argv[4]);

	int optionInd = 5;

  for(int i=optionInd;i<argc;i++) 
	{
		if (strcmp(argv[i],"--MinHoleSpanSourceDefault") == 0) {
			global->minHoleSpanSourceDefault = atoi(argv[++i]);
			if (global->minHoleSpanSourceDefault < 1) {
				cerr << "extract error: --minHoleSourceDefault should be at least 1" << endl;
				exit(1);
			}
		}
		else if (strcmp(argv[i],"--MaxHoleSpanSourceDefault") == 0) {
			global->maxHoleSpanSourceDefault = atoi(argv[++i]);
			if (global->maxHoleSpanSourceDefault < 1) {
				cerr << "extract error: --maxHoleSourceDefault should be at least 1" << endl;
				exit(1);
			}
		}
		else  if (strcmp(argv[i],"--MinHoleSpanSourceSyntax") == 0) {
			global->minHoleSpanSourceSyntax = atoi(argv[++i]);
			if (global->minHoleSpanSourceSyntax < 1) {
				cerr << "extract error: --minHoleSourceSyntax should be at least 1" << endl;
				exit(1);
			}
		}
		else if (strcmp(argv[i],"--UppermostOnly") == 0) {
			global->uppermostOnly = atoi(argv[++i]);
		}
		else if (strcmp(argv[i],"--MaxHoleSpanSourceSyntax") == 0) {
			global->maxHoleSpanSourceSyntax = atoi(argv[++i]);
			if (global->maxHoleSpanSourceSyntax < 1) {
				cerr << "extract error: --maxHoleSourceSyntax should be at least 1" << endl;
				exit(1);
			}
		}
		
		// maximum number of words in hierarchical phrase
		else if (strcmp(argv[i],"--maxSymbolsSource") == 0) {
			global->maxSymbolsSource = atoi(argv[++i]);
			if (global->maxSymbolsSource < 1) {
				cerr << "extract error: --maxSymbolsSource should be at least 1" << endl;
				exit(1);
			}
		}

                // maximum number of words in hierarchical phrase                                                                           
                else if (strcmp(argv[i],"--maxSymbolsTarget") == 0) {
		    global->maxSymbolsTarget = atoi(argv[++i]);
		    if (global->maxSymbolsTarget < 1) {
			cerr << "extract error: --maxSymbolsTarget should be at least 1" << endl;
			exit(1);
		    }
                }
		// maximum number of non-terminals
		else if (strcmp(argv[i],"--MaxNonTerm") == 0) {
			global->maxNonTerm = atoi(argv[++i]);
			if (global->maxNonTerm < 1) {
				cerr << "extract error: --MaxNonTerm should be at least 1" << endl;
				exit(1);
			}
		}		
		// allow consecutive non-terminals (X Y | X Y)
    else if (strcmp(argv[i],"--TargetSyntax") == 0) {
      global->targetSyntax = true;
    }
    else if (strcmp(argv[i],"--SourceSyntax") == 0) {
      global->sourceSyntax = true;
    }
		// do not create many part00xx files!
    else if (strcmp(argv[i],"--NoFileLimit") == 0) {
      // now default
    }
		else if (strcmp(argv[i],"--GlueGrammar") == 0) {
			global->glueGrammarFlag = true;
			if (++i >= argc)
			{
				cerr << "ERROR: Option --GlueGrammar requires a file name" << endl;
				exit(0);
			}
			fileNameGlueGrammar = string(argv[i]);
			cerr << "creating glue grammar in '" << fileNameGlueGrammar << "'" << endl;
    }
		else if (strcmp(argv[i],"--UnknownWordLabel") == 0) {
			global->unknownWordLabelFlag = true;
			if (++i >= argc)
			{
				cerr << "ERROR: Option --UnknownWordLabel requires a file name" << endl;
				exit(0);
			}
			fileNameUnknownWordLabel = string(argv[i]);
			cerr << "creating unknown word labels in '" << fileNameUnknownWordLabel << "'" << endl;
		}
		// TODO: this should be a useful option
    //else if (strcmp(argv[i],"--ZipFiles") == 0) {
    //  zipFiles = true;
    //}
		// if an source phrase is paired with two target phrases, then count(t|s) = 0.5
    else if (strcmp(argv[i],"--Mixed") == 0) {
			global->mixed = true;
    }
		else if (strcmp(argv[i],"--AllowDefaultNonTermEdge") == 0) {
			global->allowDefaultNonTermEdge = atoi(argv[++i]);
    }
		else if (strcmp(argv[i], "--GZOutput") == 0) {
      global->gzOutput = true;
    }
		else if (strcmp(argv[i],"--MaxSpan") == 0) {
		  // ignore
      ++i;
		}
    else if (strcmp(argv[i],"--SentenceOffset") == 0) {
      if (i+1 >= argc || argv[i+1][0] < '0' || argv[i+1][0] > '9') {
        cerr << "extract: syntax error, used switch --SentenceOffset without a number" << endl;
        exit(1);
      }
      sentenceOffset = atoi(argv[++i]);
    }
    else {
      cerr << "extract: syntax error, unknown option '" << string(argv[i]) << "'\n";
      exit(1);
    }
  }


	// open input files
	Moses::InputFileStream tFile(fileNameT);
	Moses::InputFileStream sFile(fileNameS);
	Moses::InputFileStream aFile(fileNameA);

	// open output files
  string fileNameExtractInv = fileNameExtract + ".inv";
  if (global->gzOutput) {
    fileNameExtract += ".gz";
    fileNameExtractInv += ".gz";
  }

  Moses::OutputFileStream extractFile;
  Moses::OutputFileStream extractFileInv;
  extractFile.Open(fileNameExtract.c_str());
  extractFileInv.Open(fileNameExtractInv.c_str());
  
  
	// loop through all sentence pairs
  int i = sentenceOffset;
  while(true) {
    i++;

    if (i % 1000 == 0) {
      cerr << i << " " << flush;
    }

    string targetString;
    string sourceString;
    string alignmentString;
		
		bool ok = getline(tFile, targetString);
		if (!ok)
			break;
		getline(sFile, sourceString);
		getline(aFile, alignmentString);
    
		//cerr << endl << targetString << endl << sourceString << endl << alignmentString << endl;

		//time_t currTime = time(NULL);
		//cerr << "A " << (currTime - starttime) << endl;

    SentenceAlignment sentencePair;
    if (sentencePair.Create( targetString, sourceString, alignmentString, i, *global )) 
		{			
			//cerr << sentence.sourceTree << endl;
			//cerr << sentence.targetTree << endl;

			sentencePair.FindTunnels(*g_global);
			//cerr << "C " << (time(NULL) - starttime) << endl;
			//cerr << sentencePair << endl;
			
			sentencePair.CreateLattice(*g_global);
			//cerr << "D " << (time(NULL) - starttime) << endl;
			//cerr << sentencePair << endl;

			sentencePair.CreateRules(*g_global);
			//cerr << "E " << (time(NULL) - starttime) << endl;

			//cerr << sentence.lattice->GetRules().GetSize() << endl;
			sentencePair.GetLattice().GetRules().Output(extractFile);
      sentencePair.GetLattice().GetRules().OutputInv(extractFileInv);
    }
  }
	
  tFile.Close();
  sFile.Close();
  aFile.Close();

  extractFile.Close();
  extractFileInv.Close();

  if (global->glueGrammarFlag) {
    writeGlueGrammar(fileNameGlueGrammar, *global, targetLabelCollection, targetTopLabelCollection);
  }

  delete global;
}
 

void writeGlueGrammar( const string & fileName, Global &options, set< string > &targetLabelCollection, map< string, int > &targetTopLabelCollection )
{
  ofstream grammarFile;
  grammarFile.open(fileName.c_str());
  if (!options.targetSyntax) {
    grammarFile << "<s> [X] ||| <s> [S] ||| 1 ||| ||| 0" << endl
                << "[X][S] </s> [X] ||| [X][S] </s> [S] ||| 1 ||| 0-0 ||| 0" << endl
                << "[X][S] [X][X] [X] ||| [X][S] [X][X] [S] ||| 2.718 ||| 0-0 1-1 ||| 0" << endl;
  } else {
    // chose a top label that is not already a label
    string topLabel = "QQQQQQ";
    for( unsigned int i=1; i<=topLabel.length(); i++) {
      if(targetLabelCollection.find( topLabel.substr(0,i) ) == targetLabelCollection.end() ) {
        topLabel = topLabel.substr(0,i);
        break;
      }
    }
    // basic rules
    grammarFile << "<s> [X] ||| <s> [" << topLabel << "] ||| 1  ||| " << endl
                << "[X][" << topLabel << "] </s> [X] ||| [X][" << topLabel << "] </s> [" << topLabel << "] ||| 1 ||| 0-0 " << endl;

    // top rules
    for( map<string,int>::const_iterator i =  targetTopLabelCollection.begin();
         i !=  targetTopLabelCollection.end(); i++ ) {
      grammarFile << "<s> [X][" << i->first << "] </s> [X] ||| <s> [X][" << i->first << "] </s> [" << topLabel << "] ||| 1 ||| 1-1" << endl;
    }

    // glue rules
    for( set<string>::const_iterator i =  targetLabelCollection.begin();
         i !=  targetLabelCollection.end(); i++ ) {
      grammarFile << "[X][" << topLabel << "] [X][" << *i << "] [X] ||| [X][" << topLabel << "] [X][" << *i << "] [" << topLabel << "] ||| 2.718 ||| 0-0 1-1" << endl;
    }
    grammarFile << "[X][" << topLabel << "] [X][X] [X] ||| [X][" << topLabel << "] [X][X] [" << topLabel << "] ||| 2.718 |||  0-0 1-1 " << endl; // glue rule for unknown word...
  }
  grammarFile.close();
}

