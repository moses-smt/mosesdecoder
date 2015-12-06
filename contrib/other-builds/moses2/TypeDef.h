/*
 * TypeDef.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <cstddef>

class Hypothesis;

#ifndef BOS_
#define BOS_ "<s>" //Beginning of sentence symbol
#endif
#ifndef EOS_
#define EOS_ "</s>" //End of sentence symbol
#endif

typedef float SCORE;

// Note: StaticData uses SearchAlgorithm to determine whether the translation
// model is phrase-based or syntax-based.  If you add a syntax-based search
// algorithm here then you should also update StaticData::IsSyntax().
enum SearchAlgorithm {
  Normal = 0,
  CubePruning	= 1,
  //,CubeGrowing = 2
  CYKPlus = 3,
  NormalBatch  = 4,
  ChartIncremental = 5,
  SyntaxS2T = 6,
  SyntaxT2S = 7,
  SyntaxT2S_SCFG = 8,
  SyntaxF2S = 9,
  DefaultSearchAlgorithm = 777 // means: use StaticData.m_searchAlgorithm
};

class StackAdd
{
public:
	bool added;
	Hypothesis *toBeDeleted;

	StackAdd(bool vadded,
			Hypothesis *vtoBeDeleted)
	:added(vadded)
	,toBeDeleted(vtoBeDeleted)
	{
	}
};



