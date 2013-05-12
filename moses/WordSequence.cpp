//Fabienne Braune
//Sequence of Words for decoding with l-MBOT rules
//

#include <vector>
#include "WordSequence.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

	WordSequence::~WordSequence()
	{
		RemoveAllInColl(*m_wordSequence);
	};

	void WordSequence::Add(Word * Word)
	{
		m_wordSequence->push_back(Word);
	};

	vector<Word*> *WordSequence::GetSequence()
	{
		return m_wordSequence;
	};


	Word *WordSequence::GetWord(size_t position)
	{
		CHECK(position < GetSize());
		return m_wordSequence->at(position);
	};

	size_t WordSequence::GetSize()
	{
		return m_wordSequence->size();
	};

}
