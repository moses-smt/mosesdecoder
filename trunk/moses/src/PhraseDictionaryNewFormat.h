// $Id: PhraseDictionaryNewFormat.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $
// vim:tabstop=2

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

#include "PhraseDictionary.h"
#include "PhraseDictionaryNodeNewFormat.h"
#include "ChartRuleCollection.h"
#include "CellCollection.h"

namespace Moses
{
	class ProcessedRuleStack;
	class ProcessedRuleColl;	
	
	/*** Implementation of a phrase table in a trie.  Looking up a phrase of
	 * length n words requires n look-ups to find the TargetPhraseCollection.
	 */
	class PhraseDictionaryNewFormat : public PhraseDictionary
		{
			typedef PhraseDictionary MyBase;
			friend std::ostream& operator<<(std::ostream&, const PhraseDictionaryNewFormat&);
			
		protected:
			PhraseDictionaryNodeNewFormat m_collection;
			mutable std::vector<ChartRuleCollection*> m_chartTargetPhraseColl;
			mutable std::vector<ProcessedRuleStack*>	m_runningNodesVec;
			
			Phrase									m_prevSource;
			TargetPhraseCollection	*m_prevPhraseColl;
			
			std::string m_filePath; 
			
			TargetPhraseCollection &GetOrCreateTargetPhraseCollection(const Phrase &source, const TargetPhrase &target);
			PhraseDictionaryNodeNewFormat &GetOrCreateNode(const Phrase &source, const TargetPhrase &target);
			
			bool Load(const std::vector<FactorType> &input
								, const std::vector<FactorType> &output
								, std::istream &inStream
								, const std::vector<float> &weight
								, size_t tableLimit
								, const LMList &languageModels
								, float weightWP);
						
			void 	DeleteDuplicates(ProcessedRuleColl &nodes) const; // keep only backoff, if it exists
			
			void CreateSourceLabels(std::vector<Word> &sourceLabels
															, const std::vector<std::string> &sourceLabelsStr) const;
			Word CreateCoveredWord(const Word &origSourceLabel, const InputType &src, const WordsRange &range) const;
			
		public:
			PhraseDictionaryNewFormat(size_t numScoreComponent, PhraseDictionaryFeature* feature)
			: MyBase(numScoreComponent, feature)
			, m_prevSource(Input)
			, m_prevPhraseColl(NULL)
			{
			}
			virtual ~PhraseDictionaryNewFormat();
			
			std::string GetScoreProducerDescription() const
			{ return "Hieu's Reordering Model"; }
			
			PhraseTableImplementation GetPhraseTableImplementation() const
			{ return Memory; }
			
			bool Load(const std::vector<FactorType> &input
								, const std::vector<FactorType> &output
								, const std::string &filePath
								, const std::vector<float> &weight
								, size_t tableLimit
								, const LMList &languageModels
						    , float weightWP);
			
			const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase &source) const;
			
			void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);
			
			void AddEquivPhrase(TargetPhraseCollection	&targetPhraseColl, TargetPhrase *targetPhrase);
			
			// for mert
			void SetWeightTransModel(const std::vector<float> &weightT);
			
			TO_STRING();
			
			void InitializeForInput(const InputType& i);
			
			const ChartRuleCollection *GetChartRuleCollection(
																												InputType const& src
																												,WordsRange const& range
																												,bool adhereTableLimit
																												,const CellCollection &cellColl) const;
			
			void CleanUp();
		};
	
}
