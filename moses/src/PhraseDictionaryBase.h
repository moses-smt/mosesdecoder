#pragma once

#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Dictionary.h"
#include "TargetPhraseCollection.h"

class StaticData;
class InputType;
class WordsRange;

class PhraseDictionaryBase : public Dictionary, public ScoreProducer
{
 protected:
	size_t m_maxTargetPhrase;
	std::string m_filename;    // just for debugging purposes

 public:
	PhraseDictionaryBase(size_t noScoreComponent);
	virtual ~PhraseDictionaryBase();
		
	DecodeType GetDecodeType() const
	{
		return Translate;
	}
	
	virtual void InitializeForInput(InputType const&) {}
	const std::string GetScoreProducerDescription() const;
	unsigned int GetNumScoreComponents() const;

	virtual void SetWeightTransModel(const std::vector<float> &weightT)=0;

	virtual const TargetPhraseCollection *GetTargetPhraseCollection(const Phrase& src) const=0;
	virtual const TargetPhraseCollection *GetTargetPhraseCollection(InputType const& src,WordsRange const& range) const;

	virtual void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)=0;
};
