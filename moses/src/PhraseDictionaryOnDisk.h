// $Id: PhraseDictionaryNewFormat.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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

#include <map>
#include <vector>
#include <string>
#include "PhraseDictionary.h"
#include "ChartRuleCollection.h"
#include "../../OnDiskPt/src/OnDiskWrapper.h"
#include "../../OnDiskPt/src/Word.h"
#include "../../OnDiskPt/src/PhraseNode.h"

namespace Moses
{
class TargetPhraseCollection;
class ProcessedRuleStackOnDisk;
class CellCollection;

class PhraseDictionaryOnDisk : public PhraseDictionary
{
	typedef PhraseDictionary MyBase;
	friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryOnDisk&);
	
protected:
	std::vector<FactorType> m_inputFactorsVec, m_outputFactorsVec;
	std::vector<float> m_weight;
	std::string m_filePath;
	
	mutable OnDiskPt::OnDiskWrapper m_dbWrapper;

	mutable std::map<UINT64, const TargetPhraseCollection*> m_cache;
	mutable std::vector<ChartRuleCollection*> m_chartTargetPhraseColl;
	mutable std::list<Phrase*> m_sourcePhrase;
	mutable std::list<const OnDiskPt::PhraseNode*> m_sourcePhraseNode;

	mutable std::vector<ProcessedRuleStackOnDisk*>	m_runningNodesVec;
	
	void LoadTargetLookup();
	
public:
	PhraseDictionaryOnDisk(size_t numScoreComponent, PhraseDictionaryFeature* feature)
	: MyBase(numScoreComponent, feature)
	{}
	virtual ~PhraseDictionaryOnDisk();

	PhraseTableImplementation GetPhraseTableImplementation() const
	{ return OnDisk; }

	bool Load(const std::vector<FactorType> &input
						, const std::vector<FactorType> &output
						, const std::string &filePath
						, const std::vector<float> &weight
						, size_t tableLimit);
	
	std::string GetScoreProducerDescription() const
	{ return "BerkeleyPt"; }

	// PhraseDictionary impl
	// for mert
	void SetWeightTransModel(const std::vector<float> &weightT);
	//! find list of translations that can translates src. Only for phrase input
	virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const;

	void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);

	//! Create entry for translation of source to targetPhrase
	virtual void AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase);
	
	virtual const ChartRuleCollection *GetChartRuleCollection(InputType const& src, WordsRange const& range,
																														bool adhereTableLimit,const CellCollection &cellColl) const;
	
	void InitializeForInput(const InputType& input);
	void CleanUp();
	
};


};


