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

#include <string>
#include <vector>
#include <map>
#include "Factor.h"
#include "Phrase.h"
#include "TypeDef.h"
#include "Util.h"
#include "WordsRange.h"
#include "ScoreProducer.h"

class FactorCollection;
class Factor;
class Phrase;
class Hypothesis;
using namespace std;

/** The LexicalReordering class handles everything involved with
 * lexical reordering. It loads a probability table P(orientation|f,e)
 * and computes scores in either forward, backward, or bidirectional
 * direction. 
 * This model is described in Koehn et al. [IWSLT 2005]
 */

class LexicalReordering : public ScoreProducer
{

private: 

	// This stores the model table
	typedef std::map<std::string, std::vector<float> > ORIENTATION_TABLE;

	// This is the order in which pieces appear in the orientation table
	// when conditioning on f and e.
	enum FEFileFormat { FE_FOREIGN, FE_ENGLISH, FE_PROBS };

	// This is the order in which pieces appear in the orientation table
	// when conditioning on f only.
	enum FFileFormat { F_FOREIGN, F_PROBS };

	// different numbers of probabilities for different ranges of
	// orientation variable
	static const unsigned int MSD_NUM_PROBS = 3;
	static const unsigned int MONO_NUM_PROBS = 2;

	static const unsigned int ORIENTATION_MONOTONE = 0;
	static const unsigned int ORIENTATION_NON_MONOTONE = 1;
	static const unsigned int ORIENTATION_SWAP = 1;
	static const unsigned int ORIENTATION_DISCONTINUOUS = 2;

	int m_orientation; /**< msd or monotone */
	std::vector<int> m_direction;   /**< contains forward, backward, or both (bidirectional) */
	int m_condition;   /**< fe or f */
	int m_numScores;   /**< 1, 2, 3, or 6 */
	int m_numOrientationTypes; /**< 2(mono) or 3(msd) */
	std::string m_filename; /**< probability table location */
	vector<FactorType> m_sourceFactors; /**< source factors to condition on */
	vector<FactorType> m_targetFactors; /**< target factors to condition on */


	ORIENTATION_TABLE m_orientation_table; /**< probability table */

	// Functions
	void LoadFile(void);

public:
	// Constructor: takes 3 arguments -- filename is the path to the
	// orientation probability table, orientation is one of {MSD, MONO},
	// direction is one of {FOR,BACK,BI}, and condition is one of {F,FE}.
	LexicalReordering(const std::string &filename, int orientation, int direction, 
										int condition, const std::vector<float>& weights,
										vector<FactorType> input, vector<FactorType> output);
	
	// Descructor
	~LexicalReordering(void) {}

	// Compute Orientation
	int GetOrientation(const Hypothesis *curr_hypothesis);

	// Compute and return a score for a hypothesis
	std::vector<float> CalcScore(Hypothesis *curr_hypothesis);
	
	// Print the orientation probability table
	void PrintTable(void);
	
	void SetForwardWeight(float weight);
	
	void SetBackwardWeight(float weight);
	
	float GetForwardWeight(void);
	
	float GetBackwardWeight(void);
	
	float GetProbability(Hypothesis *hypothesis, int orientation);

	size_t GetNumScoreComponents() const;
	const std::string GetScoreProducerDescription() const;
};
