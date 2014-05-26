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

#ifndef moses_TypeDef_h
#define moses_TypeDef_h

#include <list>
#include <limits>
#include <vector>
#include <string>

//! all the typedefs and enums goes here

#ifdef WIN32
#include <BaseTsd.h>
#else
#include <stdint.h>
typedef uint32_t UINT32;
typedef uint64_t UINT64;
#endif

namespace Moses
{

#define PROJECT_NAME		"moses"

#ifndef BOS_
#define BOS_ "<s>" //Beginning of sentence symbol
#endif
#ifndef EOS_
#define EOS_ "</s>" //End of sentence symbol
#endif

#define UNKNOWN_FACTOR	"UNK"
#define EPSILON         "*EPS*"

#define NOT_FOUND 			std::numeric_limits<size_t>::max()
#define MAX_NGRAM_SIZE  20

const size_t DEFAULT_CUBE_PRUNING_POP_LIMIT = 1000;
const size_t DEFAULT_CUBE_PRUNING_DIVERSITY = 0;
const size_t DEFAULT_MAX_HYPOSTACK_SIZE = 200;
const size_t DEFAULT_MAX_TRANS_OPT_CACHE_SIZE = 10000;
const size_t DEFAULT_MAX_TRANS_OPT_SIZE	= 5000;
const size_t DEFAULT_MAX_PART_TRANS_OPT_SIZE = 10000;
const size_t DEFAULT_MAX_PHRASE_LENGTH = 20;
const size_t DEFAULT_MAX_CHART_SPAN			= 10;
const size_t ARRAY_SIZE_INCR					= 10; //amount by which a phrase gets resized when necessary
const float LOWEST_SCORE							= -100.0f;
const float DEFAULT_BEAM_WIDTH				= 0.00001f;
const float DEFAULT_EARLY_DISCARDING_THRESHOLD		= 0.0f;
const float DEFAULT_TRANSLATION_OPTION_THRESHOLD	= 0.0f;
const size_t DEFAULT_VERBOSE_LEVEL = 1;

// enums.
// must be 0, 1, 2, ..., unless otherwise stated

// can only be 2 at the moment
const int NUM_LANGUAGES = 2;

// Looking for MAX_NUM_FACTORS?  It's defined by the build system: bjam --max-factors=4

enum FactorDirection {
  Input,			//! Source factors
  Output			//! Target factors
};

enum DecodeType {
  Translate
  ,Generate
  ,InsertNullFertilityWord //! an optional step that attempts to insert a few closed-class words to improve LM scores
};

namespace LexReorderType
{
enum LexReorderType { // explain values
  Backward
  ,Forward
  ,Bidirectional
  ,Fe
  ,F
};
}

namespace DistortionOrientationType
{
enum DistortionOrientationOptions {
  Monotone, //distinguish only between monotone and non-monotone as possible orientations
  Msd //further separate non-monotone into swapped and discontinuous
};
}

enum PhraseTableImplementation {
  Memory				= 0
  ,Binary				= 1
  ,OnDisk				= 2
  //,GlueRule		= 3
  //,Joshua			= 4
  //,MemorySourceLabel	= 5
  ,SCFG					= 6
  //,BerkeleyDb	= 7
  ,SuffixArray	= 8
  ,Hiero        = 9
  ,ALSuffixArray = 10
  ,FuzzyMatch    = 11
  ,Compact      = 12
  ,Interpolated = 13
  ,DSuffixArray = 14
  ,MemMappedSA = 15
};

enum InputTypeEnum {
  SentenceInput						= 0
  ,ConfusionNetworkInput	= 1
  ,WordLatticeInput				= 2
  ,TreeInputType					= 3
  ,WordLatticeInput2			= 4

};

enum XmlInputType {
  XmlPassThrough = 0,
  XmlIgnore      = 1,
  XmlExclusive   = 2,
  XmlInclusive   = 3,
  XmlConstraint	 = 4
};

enum DictionaryFind {
  Best		= 0
  ,All		= 1
};

enum SearchAlgorithm {
  Normal				= 0
  ,CubePruning	= 1
  ,CubeGrowing	= 2
  ,ChartDecoding= 3
  ,NormalBatch  = 4
  ,ChartIncremental = 5
};

enum SourceLabelOverlap {
  SourceLabelOverlapAdd = 0
  ,SourceLabelOverlapReplace = 1
  ,SourceLabelOverlapDiscard = 2
};

enum WordAlignmentSort {
  NoSort = 0
  ,TargetOrder = 1
};

enum FormatType {
  MosesFormat
  ,HieroFormat
};

// typedef
typedef size_t FactorType;

typedef std::vector<float> Scores;
typedef std::vector<std::string> WordAlignments;

typedef std::vector<FactorType> FactorList;

typedef std::pair<std::vector<std::string const*>,WordAlignments > StringWordAlignmentCand;

}
#endif
