
#pragma once

#include <vector>
#include "Search.h"
#include "HypothesisStack.h"

class InputType;
class TranslationOptionCollection;

class SearchCubePruning: public Search
{
protected:
		const InputType &m_source;
		std::vector < HypothesisStack* > m_hypoStackColl; /**< stacks to store hypotheses (partial translations) */ 
	// no of elements = no of words in source + 1
	TranslationOptionCollection *m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */
	TargetPhrase m_initialTargetPhrase; /**< used to seed 1st hypo */

	//! go thru all bitmaps in 1 stack & create backpointers to bitmaps in the stack
	void CreateForwardTodos(HypothesisStack &stack);
	//! create a back pointer to this bitmap, with edge that has this words range translation
	void CreateForwardTodos(const WordsBitmap &bitmap, const WordsRange &range, BitmapContainer &bitmapContainer);
	bool CheckDistortion(const WordsBitmap &bitmap, const WordsRange &range) const;

	void PrintBitmapContainerGraph();

public:
	SearchCubePruning(const InputType &source);
	~SearchCubePruning();

	void ProcessSentence();

	void OutputHypoStackSize();
	void OutputHypoStack(int stack);

	virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const;
	virtual const Hypothesis *GetBestHypothesis() const;
};

