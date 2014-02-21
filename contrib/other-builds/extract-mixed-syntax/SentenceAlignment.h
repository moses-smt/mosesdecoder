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
#include <cassert>
#include <iostream>
#include "SyntaxTree.h"
#include "Global.h"
#include "Range.h"

class TunnelCollection;
class Lattice;

class SentenceAlignment 
{
	friend std::ostream& operator<<(std::ostream&, const SentenceAlignment&);

public:
  std::vector<std::string> target;
  std::vector<std::string> source;
  std::vector<int> alignedCountS;
  std::vector< std::vector<int> > alignedToT;
  SyntaxTree sourceTree, targetTree;
	
	//typedef std::vector<Range> Inner;
	//typedef std::vector<Inner> Outer;
	
	//Outer m_s2tTightest, m_t2sTightest;
	
	SentenceAlignment();
	~SentenceAlignment();
  int Create(const std::string &targetString, const std::string &sourceString, const std::string &alignmentString, int sentenceID, const Global &global);
  //  void clear() { delete(alignment); };
	void FindTunnels( const Global &global ) ;

	void CreateLattice(const Global &global);
	void CreateRules(const Global &global);
		
	const TunnelCollection &GetTunnelCollection() const
	{ 
		assert(m_tunnelCollection);
		return *m_tunnelCollection;
	}

	const Lattice &GetLattice() const
	{ 
		assert(m_lattice);
		return *m_lattice;
	}
	
protected:
	TunnelCollection *m_tunnelCollection;
	Lattice *m_lattice;
	
	/*
	void CalcTightestSpan(Outer &tightest);
	void InitTightest(Outer &tightest, size_t len);
	Range &GetTightest(Outer &tightest, size_t startPos, size_t endPos);
	void SetAlignment(size_t source, size_t target);
	void SetAlignment(Outer &tightest, size_t thisPos, size_t thatPos);
	*/
};

