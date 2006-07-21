// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#pragma once

#include <list>
#include <limits>

#define PROJECT_NAME		"moses"

#define SENTENCE_START	"<s>"
#define SENTENCE_END		"</s>"
#define UNKNOWN_FACTOR	"UNK"

#define NOT_FOUND 			std::numeric_limits<size_t>::max()
#define MAX_NGRAM_SIZE  20

const size_t DEFAULT_MAX_HYPOSTACK_SIZE = 200;
const size_t ARRAY_SIZE_INCR					= 20; //amount by which a hypostack gets resized when necessary
const float LOWEST_SCORE							= -100.0f;
const float DEFAULT_BEAM_THRESHOLD		= 0.00001f;
const size_t DEFAULT_VERBOSE_LEVEL = 1;

///////////////////////////////////////////////// 
// for those using autoconf/automake
#if HAVE_CONFIG_H
#include "config.h"

#define TRACE_ENABLE 1		// REMOVE after we figure this out
#define N_BEST 1					// REMOVE

#  ifdef HAVE_SRILM
#    define LM_SRI 1
#    undef LM_INTERNAL
#  else
#    undef LM_SRI
#    define LM_INTERNAL 1
#  endif

#  ifdef HAVE_IRSTLM
#    define LM_IRST 1
#    undef LM_INTERNAL
#    undef LM_SRI
#  endif

#endif
///////////////////////////////////////////////// 

class NGramNode;
union LmId {
  unsigned int sri;
  const NGramNode* internal;
  int irst;
  public:
    LmId() {};
    LmId(int i) { irst = i; };
};

// enums. 
// must be 0, 1, 2, ..., unless otherwise stated

// can only be 2 at the moment
const int NUM_LANGUAGES = 2;

enum FactorType
{
	Surface				= 0
	,POS					= 1
	,Stem					= 2
	,Morphology		= 3
};

// count of above
const size_t NUM_FACTORS = 4;

enum FactorDirection
{	
	Input					= 0
	,Output				= 1
};

enum DecodeType
{
	Translate
	,Generate
  ,InsertNullFertilityWord //! an optional step that attempts to insert a few closed-class words to improve LM scores
};

namespace ScoreType {
	enum ScoreType
	{
		PhraseTrans = 0,
		Generation,
		LanguageModelScore,
		Distortion,
		WordPenalty,
		FutureScoreEnum,
		LexicalReordering,
		Total
	};
}

// count of above
const size_t NUM_SCORES = 8;

namespace LexReorderType
{
	enum LexReorderType
		{
			Monotone //TODO what the jiggers do these symbols mean?
			,Msd
			,Forward
			,Backward
			,Bidirectional
			,Fe
			,F
		};
}

enum IOMethod
{
	IOMethodCommandLine
	,IOMethodFile
	,IOMethodMySQL
};

enum LMListType
{
	Initial
	,Other
};

// typedef
class Factor;
typedef const Factor * FactorArray[NUM_FACTORS];

class LanguageModel;
typedef std::list < LanguageModel* >		LMList;

