
#include "TypeDef.h"
#include "ScoreComponent.h"
#include "Dictionary.h"

ScoreComponent::ScoreComponent()
:m_dictionary(NULL)
{ // used by collection
}

ScoreComponent::ScoreComponent(const Dictionary *dictionary)
	:m_dictionary(dictionary)
	,m_scoreComponent(dictionary->GetNoScoreComponent())
{
}

ScoreComponent::ScoreComponent(const ScoreComponent &copy)
:m_dictionary(copy.m_dictionary)
{
	if (m_dictionary != NULL)
	{
		const size_t noScoreComponent = GetNoScoreComponent();
		for (size_t i = 0 ; i < noScoreComponent ; i++)
		{
			m_scoreComponent.push_back(copy[i]);
		}
	}
}

size_t ScoreComponent::GetNoScoreComponent() const
{
	if (m_dictionary != NULL)
		return m_dictionary->GetNoScoreComponent();
	else
		return 0;
}

void ScoreComponent::Reset()
{
	if (m_dictionary != NULL)
	{
		const size_t noScoreComponent = GetNoScoreComponent();
		for (size_t i = 0 ; i < noScoreComponent ; i++)
		{
			m_scoreComponent[i] = 0;
		}
	}
}

std::ostream& operator<<(std::ostream &out, const ScoreComponent &scoreComponent)
{
	const size_t noScoreComponent = scoreComponent.GetNoScoreComponent();
	if (noScoreComponent > 0)
	{
		out << scoreComponent[0];
		for (size_t i = 1 ; i < noScoreComponent ; i++)
		{
			out << "," << scoreComponent[i];
		}
	}
	return out;
}

