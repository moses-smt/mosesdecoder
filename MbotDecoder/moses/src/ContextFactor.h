#ifndef moses_ContextFactor_h
#define moses_ContextFactor_h

#include "Word.h"
#include "Phrase.h"
#include <vector>

using namespace std;

namespace Moses
{

class ContextFactor
{
	public :

	vector<const Word*> m_contextFactors;

	//curent mbot phrases
	vector<const Phrase *> m_mbotPhrase;

	//prefix phrases : TODO : remove
	const Phrase * m_prefixPhrase;

	//prefix should be a vector of phrases
	vector<Phrase *> m_mbotPrefix;

	//position in current mbot target phrase
	size_t m_mbotPosition;
	//position in previous mbot target phrase
	size_t m_previousPosition;

	//when using MBOT components, we may have to work several time with the same previous hypothesis
	//this keeps track of it
	int m_hypoId;
	int m_seenHypo;

	int m_ngramOrder;

	ContextFactor(size_t ngramOrder,size_t mbotSize);

	size_t GetMbotPosition();
	size_t GetPreviousPosition();
	vector<const Word *> GetContextFactor();
	const Phrase * GetPhrase();
	const size_t GetPhraseSize();
	const Phrase * GetPrefixPhrase();
	const size_t GetPrefixSize();

	void SetHypoId(size_t hypoId);
	int IsSameHypoId(size_t hypoId);

	void IncrementMbotPosition();
	void IncrementPreviousPosition();
	void AddPhrase(const Phrase * current);
	void SetPrefix(const Phrase * prefix);

	void AddWordFromCurrent(size_t position);
	void AddWordFromPrefix(size_t position);
	bool IsContextEmpty();
	void ShiftAndPop();
	size_t GetNumberWords();
	void Clear();
	void ResetMbotPosition();
	void ResetPreviousPosition();
	void ShiftOrPushFromCurrent(size_t position);
	void ShiftOrPushFromPrefix(size_t position);

	//Produce context Factor vector for window

	~ContextFactor(); //delete
};


}//end of namespace


#endif
