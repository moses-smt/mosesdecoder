/*
 * TypeDef.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <cstddef>
#include <vector>
#include <istream>
#include "Vector.h"

namespace Moses2
{

class HypothesisBase;

#define NOT_FOUND 			std::numeric_limits<size_t>::max()
const size_t DEFAULT_MAX_PHRASE_LENGTH = 20;
const size_t DEFAULT_MAX_CHART_SPAN     = 20;
const size_t DEFAULT_MAX_HYPOSTACK_SIZE = 200;
const size_t DEFAULT_CUBE_PRUNING_POP_LIMIT = 1000;
const size_t DEFAULT_CUBE_PRUNING_DIVERSITY = 0;
const size_t DEFAULT_MAX_TRANS_OPT_SIZE = 5000;

const size_t DEFAULT_MAX_PART_TRANS_OPT_SIZE = 10000;
const size_t DEFAULT_MAX_TRANS_OPT_CACHE_SIZE = 10000;
const float LOWEST_SCORE = -100.0f;

const float DEFAULT_BEAM_WIDTH        = 0.00001f;
const float DEFAULT_EARLY_DISCARDING_THRESHOLD    = 0.0f;
const float DEFAULT_TRANSLATION_OPTION_THRESHOLD  = 0.0f;

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
  Normal = 0, CubePruning = 1,
  //,CubeGrowing = 2
  CYKPlus = 3,
  NormalBatch  = 4,
  ChartIncremental = 5,
  SyntaxS2T = 6,
  SyntaxT2S = 7,
  SyntaxT2S_SCFG = 8,
  SyntaxF2S = 9,
  CubePruningPerMiniStack = 10,
  CubePruningPerBitmap = 11,
  CubePruningCardinalStack = 12,
  CubePruningBitmapStack = 13,
  CubePruningMiniStack = 14,
  DefaultSearchAlgorithm = 777 // means: use StaticData.m_searchAlgorithm
};

enum InputTypeEnum {
  SentenceInput         = 0,
  ConfusionNetworkInput = 1,
  WordLatticeInput      = 2,
  TreeInputType         = 3,
  //,WordLatticeInput2 = 4,
  TabbedSentenceInput    = 5,
  ForestInputType        = 6
};

enum XmlInputType {
  XmlPassThrough = 0,
  XmlIgnore      = 1,
  XmlExclusive   = 2,
  XmlInclusive   = 3,
  XmlConstraint  = 4
};

enum WordAlignmentSort {
  NoSort = 0,
  TargetOrder = 1
};

enum S2TParsingAlgorithm {
  RecursiveCYKPlus,
  Scope3
};

enum SourceLabelOverlap {
  SourceLabelOverlapAdd = 0,
  SourceLabelOverlapReplace = 1,
  SourceLabelOverlapDiscard = 2
};

/////////////////////////
// MOSES2 only

class StackAdd
{
public:
  bool added;
  HypothesisBase *other;

  StackAdd() {
  }
  StackAdd(bool vadded, HypothesisBase *vOther) :
    added(vadded), other(vOther) {
  }
};

class Hypothesis;
typedef Vector<Hypothesis*> Batch;

class Factor;
typedef std::vector<const Factor*> Context;

}

