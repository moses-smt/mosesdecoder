
#include "TypeDef.h"
#include "Util.h"
#include "TransScoreComponent.h"
#include "PhraseDictionary.h"

TransScoreComponent::TransScoreComponent(const PhraseDictionary *phraseDictionary)
:	m_phraseDictionary(phraseDictionary)
{
	m_scoreComponent = (float*) malloc(sizeof(float) * phraseDictionary->GetNoScoreComponent());
}

TransScoreComponent::TransScoreComponent(const TransScoreComponent &copy)
	:m_phraseDictionary(copy.m_phraseDictionary)
{
	size_t noScoreComponent = m_phraseDictionary->GetNoScoreComponent();
	m_scoreComponent = (float*) malloc(sizeof(float) * noScoreComponent);
	
	for (size_t i = 0 ; i < noScoreComponent ; i++)
	{
		m_scoreComponent[i] = copy[i];
	}
}

TransScoreComponent::~TransScoreComponent()
{
	free(m_scoreComponent);
}

void TransScoreComponent::Reset()
{
	size_t noScoreComponent = m_phraseDictionary->GetNoScoreComponent();
	for (size_t i = 0 ; i < noScoreComponent ; i++)
	{
		m_scoreComponent[i] = 0;
	}
}

size_t TransScoreComponent::GetPhraseDictionaryId() const
{
	return m_phraseDictionary->GetId();
}

