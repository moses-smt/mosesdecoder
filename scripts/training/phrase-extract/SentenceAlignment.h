#pragma once
/*
 *  SentenceAlignment.h
 *  extract
 *
 *  Created by Hieu Hoang on 19/01/2010.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
#include <vector>
#include "SyntaxTree.h"
#include "Global.h"

class SentenceAlignment {
public:
  std::vector<std::string> target;
  std::vector<std::string> source;
  std::vector<int> alignedCountS;
  std::vector< std::vector<int> > alignedToT;
  SyntaxTree targetTree;
  SyntaxTree sourceTree;
	
  int create(char targetString[], char sourceString[], char alignmentString[], int sentenceID, const Global &global);
  //  void clear() { delete(alignment); };
};

// sentence-level collection of rules
class ExtractedRule {
public:
	std::string source,target,alignment,alignmentInv,orientation,orientationForward;
	int startT,endT,startS,endS;
	float count; 
	ExtractedRule( int sT,int eT,int sS,int eS )
	:startT(sT),endT(eT),startS(sS),endS(eS) {
		source = "";
		target = "";
		alignment = "";
		alignmentInv = "";
		orientation = "";
		orientationForward = "";
		count = 0;
		// countInv = 0;
	}
};
