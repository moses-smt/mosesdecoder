
#include "TypeDef.h"
#include "Util.h"
#include "TransScoreComponent.h"
#include "PhraseDictionary.h"

TransScoreComponent::TransScoreComponent(const PhraseDictionary *phraseDictionary)
{
	m_phraseDictionary = phraseDictionary;
}

TransScoreComponent::TransScoreComponent(const TransScoreComponent &copy)
	:m_phraseDictionary(copy.m_phraseDictionary)
{
	for (size_t i = 0 ; i < NUM_PHRASE_SCORES ; i++)
	{
		m_scoreComponent[i] = copy[i];
	}
}

void TransScoreComponent::Reset()
{
	for (size_t i = 0 ; i < NUM_PHRASE_SCORES ; i++)
	{
		m_scoreComponent[i] = 0;
	}
}

size_t TransScoreComponent::GetPhraseDictionaryId() const
{
	return m_phraseDictionary->GetId();
}

