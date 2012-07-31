//
//  Util.cpp
//  fuzzy-match
//
//  Created by Hieu Hoang on 26/07/2012.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include "Util.h"
#include "SentenceAlignment.h"
#include "SuffixArray.h"

void load_corpus( const char* fileName, vector< vector< WORD_ID > > &corpus )
{ // source 
	ifstream fileStream;
	fileStream.open(fileName);
	if (!fileStream) {
		cerr << "file not found: " << fileName << endl;
		exit(1);
	}
  cerr << "loading " << fileName << endl;

	istream *fileStreamP = &fileStream;
  
	char line[LINE_MAX_LENGTH];
	while(true)
	{
		SAFE_GETLINE((*fileStreamP), line, LINE_MAX_LENGTH, '\n');
		if (fileStreamP->eof()) break;
		corpus.push_back( vocabulary.Tokenize( line ) );
	}
}

void load_target( const char* fileName, vector< vector< SentenceAlignment > > &corpus)
{ 
	ifstream fileStream;
	fileStream.open(fileName);
	if (!fileStream) {
		cerr << "file not found: " << fileName << endl;
		exit(1);
	}
  cerr << "loading " << fileName << endl;

	istream *fileStreamP = &fileStream;
  
  WORD_ID delimiter = vocabulary.StoreIfNew("|||");
  
  int lineNum = 0;
	char line[LINE_MAX_LENGTH];
	while(true)
	{
		SAFE_GETLINE((*fileStreamP), line, LINE_MAX_LENGTH, '\n');
		if (fileStreamP->eof()) break;
    
    vector<WORD_ID> toks = vocabulary.Tokenize( line );
    
    corpus.push_back(vector< SentenceAlignment >());
    vector< SentenceAlignment > &vec = corpus.back();
    
    vec.push_back(SentenceAlignment());
    SentenceAlignment *sentence = &vec.back();
    
    const WORD &countStr = vocabulary.GetWord(toks[0]);
    sentence->count = atoi(countStr.c_str());
    
    for (size_t i = 1; i < toks.size(); ++i) {
      WORD_ID wordId = toks[i];
      
      if (wordId == delimiter) {
        // target and alignments can have multiple sentences.
        vec.push_back(SentenceAlignment());
        sentence = &vec.back();
        
        // count
        ++i;
        
        const WORD &countStr = vocabulary.GetWord(toks[i]);
        sentence->count = atoi(countStr.c_str());
      }
      else {
        // just a normal word, add
        sentence->target.push_back(wordId);
      }
    }
    
    ++lineNum;
    
	}
  
}


void load_alignment( const char* fileName, vector< vector< SentenceAlignment > > &corpus )
{ 
  ifstream fileStream;
	fileStream.open(fileName);
	if (!fileStream) {
		cerr << "file not found: " << fileName << endl;
		exit(1);
	}
  cerr << "loading " << fileName << endl;

	istream *fileStreamP = &fileStream;
  
  string delimiter = "|||";
  
  int lineNum = 0;
	char line[LINE_MAX_LENGTH];
	while(true)
	{
		SAFE_GETLINE((*fileStreamP), line, LINE_MAX_LENGTH, '\n');
		if (fileStreamP->eof()) break;
    
    vector< SentenceAlignment > &vec = corpus[lineNum];
    size_t targetInd = 0;
    SentenceAlignment *sentence = &vec[targetInd];
    
    vector<string> toks = Tokenize(line);
    
    for (size_t i = 0; i < toks.size(); ++i) {
      string &tok = toks[i];
      
      if (tok == delimiter) {
        // target and alignments can have multiple sentences.
        ++targetInd;
        sentence = &vec[targetInd];
        
        ++i;
      }
      else {
        // just a normal alignment, add
        vector<int> alignPoint = Tokenize<int>(tok, "-");
        assert(alignPoint.size() == 2);
        sentence->alignment.push_back(pair<int,int>(alignPoint[0], alignPoint[1]));
      }
    }
    
    ++lineNum;
    
	}
}




