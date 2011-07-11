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

class TunnelCollection;
class Lattice;

class SentenceAlignment 
{
protected:

	
public:
  std::vector<std::string> target;
  std::vector<std::string> source;
  std::vector<int> alignedCountS;
  std::vector< std::vector<int> > alignedToT;
  SyntaxTree sourceTree, targetTree;
	TunnelCollection *holeCollection;
	Lattice *lattice;
	
	SentenceAlignment();
	~SentenceAlignment();
  int create(const std::string &targetString, const std::string &sourceString, const std::string &alignmentString, int sentenceID, const Global &global);
  //  void clear() { delete(alignment); };
	void FindAlignedHoles( const Global &global ) ;

	void CreateLattice(const Global &global);
	void CreateRules(const Global &global);
	
};

