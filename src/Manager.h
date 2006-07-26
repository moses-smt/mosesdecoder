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
#include "Input.h"
#include "Hypothesis.h"
#include "StaticData.h"
#include "TranslationOption.h"
#include "HypothesisCollection.h"
#include "HypothesisCollectionIntermediate.h"
#include "TranslationOptionCollectionText.h"
#include "LatticePathList.h"
#include "SquareMatrix.h"
#include "WordsBitmap.h"
//#include "UnknownWordHandler.h"

class LatticePath;

class Manager
{
protected:	
	// data
	InputType const& m_source;

	std::vector < HypothesisCollection > m_hypoStack;
		// no of elements = no of words in source + 1
	StaticData &m_staticData;
	TranslationOptionCollection &m_possibleTranslations;

	// functions for creating hypotheses
	void ProcessOneStack(HypothesisCollection &sourceHypoColl);
	void ProcessOneHypothesis(const Hypothesis &hypothesis);
	void CreateNextHypothesis(const Hypothesis &hypothesis, HypothesisCollectionIntermediate &outputHypoColl);

	void OutputHypoStack(int stack = -1);
	void OutputHypoStackSize();
public:
	Manager(InputType const& source, TranslationOptionCollection&, StaticData &staticData);
	~Manager();

	void ProcessSentence();
	const Hypothesis *GetBestHypothesis() const;
	void CalcNBest(size_t count, LatticePathList &ret) const;
};

