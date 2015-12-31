/*
 * TypeDef.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <cstddef>
#include <vector>

namespace Moses2
{

class Hypothesis;

#define NOT_FOUND 			std::numeric_limits<size_t>::max()
const size_t DEFAULT_MAX_PHRASE_LENGTH = 20;
const size_t DEFAULT_MAX_HYPOSTACK_SIZE = 200;
const size_t DEFAULT_CUBE_PRUNING_POP_LIMIT = 1000;
const size_t DEFAULT_MAX_TRANS_OPT_CACHE_SIZE = 10000;
const float LOWEST_SCORE					= -100.0f;

#ifndef BOS_
#define BOS_ "<s>" //Beginning of sentence symbol
#endif
#ifndef EOS_
#define EOS_ "</s>" //End of sentence symbol
#endif

typedef size_t FactorType;
typedef float SCORE;
typedef std::vector<FactorType> FactorList;

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

	StackAdd() {}
	StackAdd(bool vadded,
			Hypothesis *vtoBeDeleted)
	:added(vadded)
	,toBeDeleted(vtoBeDeleted)
	{
	}
};

}



