
#include "IntraPhraseTargetPhrase.h"

using namespace std;

IntraPhraseTargetPhrase::IntraPhraseTargetPhrase(size_t numWordsCovered)
:m_sourceCompleted(numWordsCovered)
{}

void IntraPhraseTargetPhrase::Append(const WordsRange &sourceRange, const TargetPhrase &appendPhrase)
{
	m_sourceCompleted.SetValue(sourceRange.GetStartPos(), sourceRange.GetEndPos(), true);
	TargetPhrase::Append(sourceRange, appendPhrase);
}

