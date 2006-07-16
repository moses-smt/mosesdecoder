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

#include <vector>
#include <list>
#include "Sentence.h"
#include "Hypothesis.h"
#include "StaticData.h"
#include "TranslationOption.h"
#include "HypothesisCollection.h"
#include "HypothesisCollectionIntermediate.h"
#include "TranslationOptionCollection.h"
#include "LatticePathList.h"
#include "SquareMatrix.h"
#include "WordsBitmap.h"
//#include "UnknownWordHandler.h"

class LatticePath;

class Manager
{
protected:	
	// data
	Sentence m_source;
	std::vector < HypothesisCollection > m_hypoStack;
		// no of elements = no of words in source + 1
	StaticData &m_staticData;
	TranslationOptionCollection m_possibleTranslations;

	// functions
	void ProcessOneStack(const std::list < DecodeStep > &decodeStepList
													, HypothesisCollection &sourceHypoColl);
	void ProcessOneHypothesis(const std::list < DecodeStep > &decodeStepList
													, const Hypothesis &hypothesis);
	void ProcessInitialTranslation(const Hypothesis &hypothesis
													, const DecodeStep &decodeStep
													,HypothesisCollectionIntermediate &outputHypoColl);
	void ProcessTranslation(const Hypothesis &hypothesis
													, const DecodeStep &decodeStep
													, HypothesisCollectionIntermediate &outputHypoColl);
	void ProcessGeneration(const Hypothesis &hypothesis
													, const DecodeStep &decodeStep
													, HypothesisCollectionIntermediate &outputHypoColl);
	void OutputHypoStack(int stack = -1);
	void OutputHypoStackSize();
public:
	Manager(const Sentence &sentence, StaticData &staticData);
	~Manager();

	void ProcessSentence();
	const Hypothesis *GetBestHypothesis() const;
	void CalcNBest(size_t count, LatticePathList &ret) const;
};

