// $Id$
#ifndef PHRASEDICTIONARYTREEADAPTOR_H_
#define PHRASEDICTIONARYTREEADAPTOR_H_
#include <vector>
#include "TypeDef.h"
#include "PhraseDictionary.h"
#include "CreateTargetPhraseCollection.h"

class Phrase;
class PDTAimp;

class PhraseDictionaryTreeAdaptor : public PhraseDictionaryBase {
	typedef PhraseDictionaryBase MyBase;
	PDTAimp *imp;
	friend class PDTAimp;
 public:
	PhraseDictionaryTreeAdaptor(size_t noScoreComponent);
	virtual ~PhraseDictionaryTreeAdaptor();

	// enable/disable caching
	// you enable caching if you request the target candidates for a source phrase multiple times
	// if you do caching somewhere else, disable it
	// good settings for current Moses: disable for first factor, enable for other factors
	// default: enable	

	void EnableCache();
	void DisableCache();

	// initialize ...
	void Create(const std::vector<FactorType> &input
							, const std::vector<FactorType> &output
							, FactorCollection &factorCollection
							, const std::string &filePath
							, const std::vector<float> &weight
							, size_t maxTargetPhrase
							, const LMList &languageModels
							, float weightWP
							);

	// get translation candidates for a given source phrase
	// returns null pointer if nothing found
	TargetPhraseCollection const* GetTargetPhraseCollection(Phrase const &src) const;

	// clean up temporary memory etc.
	void CleanUp();
	
	// change model scaling factors
	void SetWeightTransModel(const std::vector<float> &weightT);

	// these two function can be only used for UNKNOWN source phrases
	TargetPhraseCollection const * FindEquivPhrase(const Phrase &source) const; 
	void AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase);
};
#endif
