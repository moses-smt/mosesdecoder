//Fabienne Braune
//Sequence of phrases for decoding with l-MBOT rules
//

#include <vector>
#include "PhraseSequence.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

	PhraseSequence::~PhraseSequence()
	{
		RemoveAllInColl(*m_phraseSequence);
	};

	void PhraseSequence::Add(Phrase * phrase)
	{
		m_phraseSequence->push_back(phrase);
	};

	vector<Phrase*> *PhraseSequence::GetSequence() const
	{
		return m_phraseSequence;
	};

	Phrase *PhraseSequence::GetPhrase(size_t position) const
	{
		CHECK(position < GetSize());
		return m_phraseSequence->at(position);
	};

	size_t PhraseSequence::GetSize() const
	{
		return m_phraseSequence->size();
	};




}
