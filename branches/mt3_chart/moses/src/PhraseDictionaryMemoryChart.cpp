
#include "PhraseDictionaryMemory.h"
#include "PhraseDictionaryNode.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "ChartRuleCollection.h"

using namespace std;
using namespace Moses;


class ProcessedRule
{
protected:
	const PhraseDictionaryNode *m_lastNode;
	vector<WordsConsumed> m_wordsConsumed;
public:
	ProcessedRule(const PhraseDictionaryNode *lastNode)
		:m_lastNode(lastNode)
	{}
	ProcessedRule(const ProcessedRule &prevProcessedRule, const PhraseDictionaryNode *lastNode)
		:m_lastNode(lastNode)
		,m_wordsConsumed(prevProcessedRule.m_wordsConsumed)
	{}
	ProcessedRule(const ProcessedRule &prevProcessedRule)
		:m_lastNode(prevProcessedRule.m_lastNode)
		,m_wordsConsumed(prevProcessedRule.m_wordsConsumed)
	{}
	const PhraseDictionaryNode &GetLastNode() const
	{ return *m_lastNode; }
	const vector<WordsConsumed> &GetWordsConsumed() const
	{ return m_wordsConsumed; }
	bool IsCurrNonTerminal() const
	{
		return m_wordsConsumed.empty() ? false : m_wordsConsumed.back().IsNonTerminal();
	}

	void AddConsume(size_t pos, bool isNonTerminal)
	{
		m_wordsConsumed.push_back(WordsConsumed(WordsRange(pos, pos), isNonTerminal));
	}
	void ExtendConsume(size_t pos)
	{
		WordsRange &range = m_wordsConsumed.back().GetWordsRange();
		range.SetEndPos(range.GetEndPos() + 1);
	}
/*
	inline int Compare(const ProcessedRule &compare) const
	{
		if (m_lastNode < compare.m_lastNode)
			return -1;
		if (m_lastNode > compare.m_lastNode)
			return 1;

		return m_wordsConsumed < compare.m_wordsConsumed;
	}
	inline bool operator<(const ProcessedRule &compare) const
	{
		return Compare(compare) < 0;
	}
*/
};

const ChartRuleCollection *PhraseDictionaryMemory::GetChartRuleCollection(
																	InputType const& src
																	,WordsRange const& range
																	,bool adhereTableLimit) const
{
	ChartRuleCollection *ret = new ChartRuleCollection();
	m_chartTargetPhraseColl.push_back(ret);

	FactorCollection &factorCollection = FactorCollection::Instance();
	const Factor *nonTermFactor = factorCollection.AddFactor(Input, m_inputFactors[0], NON_TERMINAL_FACTOR);
	Word nonTermWord;

	assert(m_inputFactors[0] == true);
	nonTermWord.SetFactor(0, nonTermFactor);

	vector<	vector<ProcessedRule> > runningNodes(range.GetNumWordsCovered()+1);
	runningNodes[0].push_back(ProcessedRule(&m_collection));

	// MAIN LOOP. create list of nodes of target phrases
	size_t relPos = 0;
	for (size_t absPos = range.GetStartPos(); absPos <= range.GetEndPos(); ++absPos)
	{
		vector<ProcessedRule>
					&todoNodes = runningNodes[relPos]
					,&doneNodes	= runningNodes[relPos+1];

		vector<ProcessedRule>::iterator iterNode;
		for (iterNode = todoNodes.begin(); iterNode != todoNodes.end(); ++iterNode)
		{
			ProcessedRule &prevProcessedRule = *iterNode;
			const PhraseDictionaryNode &todoNode = prevProcessedRule.GetLastNode();

			const PhraseDictionaryNode *node = todoNode.GetChild(src.GetWord(absPos));
			if (node != NULL)
			{
				ProcessedRule processedRule(prevProcessedRule, node);
				processedRule.AddConsume(absPos, false);
				doneNodes.push_back(processedRule);
			}

			// search for X (non terminals)
			node = todoNode.GetChild(nonTermWord);
			if (node != NULL)
			{
				ProcessedRule processedRule(prevProcessedRule, node);
				processedRule.AddConsume(absPos, true);
				doneNodes.push_back(processedRule);
			}
			// add prev non-term too
			if (prevProcessedRule.IsCurrNonTerminal())
			{
				ProcessedRule processedRule(prevProcessedRule);
				processedRule.ExtendConsume(absPos);
				doneNodes.push_back(processedRule);
			}
		}

		relPos++;
	}

	// return list of target phrases
	vector<ProcessedRule> &nodes = runningNodes.back();

	vector<ProcessedRule>::iterator iterNode;
	for (iterNode = nodes.begin(); iterNode != nodes.end(); ++iterNode)
	{
		const PhraseDictionaryNode &node = iterNode->GetLastNode();
		const vector<WordsConsumed> &wordsConsumed = iterNode->GetWordsConsumed();
		const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollection();

		if (targetPhraseCollection != NULL)
		{
			ret->Add(*targetPhraseCollection, wordsConsumed, adhereTableLimit, GetTableLimit());
		}
	}

	return ret;
}
