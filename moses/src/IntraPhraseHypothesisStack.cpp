
#include "IntraPhraseHypothesisStack.h"

IntraPhraseHypothesisStack::IntraPhraseHypothesisStack(size_t maxSize)
:m_maxSize(maxSize)
{
}

void IntraPhraseHypothesisStack::AddPrune(const IntraPhraseTargetPhrase &phrase)
{
	m_coll.insert(phrase);
}

void IntraPhraseHypothesisStack::Process()
{

}
