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
#include <vector>

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

const size_t DEFAULT_MAX_HYPOSTACK_SIZE = 200;
const size_t DEFAULT_MAX_TRANS_OPT_SIZE	= 50;
const size_t DEFAULT_MAX_PART_TRANS_OPT_SIZE = 10000;
const size_t DEFAULT_MAX_PHRASE_LENGTH = 20;
const size_t ARRAY_SIZE_INCR					= 10; //amount by which a phrase gets resized when necessary
const float LOWEST_SCORE							= -100.0f;
const float DEFAULT_BEAM_THRESHOLD		= 0.00001f;
const size_t DEFAULT_VERBOSE_LEVEL = 1;

///////////////////////////////////////////////// 
// for those using autoconf/automake
#if HAVE_CONFIG_H
#include "config.h"

#define TRACE_ENABLE 1		// REMOVE after we figure this out

#  ifdef HAVE_SRILM
#    define LM_SRI 1
#  else
#    undef LM_SRI
#  endif

#  ifdef HAVE_IRSTLM
#    define LM_IRST 1
#  endif

#endif
///////////////////////////////////////////////// 

// enums. 
// must be 0, 1, 2, ..., unless otherwise stated

// can only be 2 at the moment
const int NUM_LANGUAGES = 2;

const size_t MAX_NUM_FACTORS = 4;

enum FactorDirection
{	
	Input,			//! Source factors
	Output			//! Target factors
};

enum DecodeType
{
	Translate
	,Generate
  ,InsertNullFertilityWord //! an optional step that attempts to insert a few closed-class words to improve LM scores
};

namespace LexReorderType
{
	enum LexReorderType //TODO explain values
		{
			Backward
			,Forward
			,Bidirectional
			,Fe
			,F
		};
}

namespace DistortionOrientationType
{
	enum DistortionOrientationOptions
		{
			Monotone, //distinguish only between monotone and non-monotone as possible orientations
			Msd //further separate non-monotone into swapped and discontinuous
		};	
}

enum LMType
{	
	SingleFactor
	,MultiFactor
};
enum LMImplementation
{	
	SRI				= 0 
	,IRST			= 1
	,Skip			= 2
	,Joint		= 3
	,Internal	= 4
};


enum InputTypeEnum
{
	SentenceInput						= 0
	,ConfusionNetworkInput	= 1
	,WordLatticeInput				= 2
};

enum XmlInputType
{
	XmlPassThrough = 0,
	XmlIgnore = 1,
	XmlExclusive = 2,
	XmlInclusive = 3
};

enum DictionaryFind
{	
	Best
	,All
};

enum DecoderType
{	
	MAP
	,MBR
};

// typedef
typedef size_t FactorType;

#ifdef WIN32
#include <BaseTsd.h>
#else
#include <stdint.h>
typedef uint32_t UINT32;
#endif

class TranslationOption;
typedef std::vector<TranslationOption*> TranslationOptionList;

