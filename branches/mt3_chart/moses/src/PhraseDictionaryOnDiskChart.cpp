
#include <vector>
#include "PhraseDictionaryOnDisk.h"
#include "ChartRuleCollection.h"
#include "InputType.h"
#include "FactorCollection.h"
#include "StaticData.h"
#include "../../on-disk-phrase-dict/src/SourcePhraseNode.h"
#include "../../on-disk-phrase-dict/src/TargetPhraseList.h"

using namespace std;

namespace Moses
{

class ProcessedRule
{
protected:
	MosesOnDiskPt::SourcePhraseNode m_lastNode;
	vector<WordsConsumed> m_wordsConsumed;
public:
	ProcessedRule(const MosesOnDiskPt::SourcePhraseNode &lastNode)
		:m_lastNode(lastNode)
	{}
	ProcessedRule(const ProcessedRule &prevProcessedRule, const MosesOnDiskPt::SourcePhraseNode &lastNode)
		:m_lastNode(lastNode)
		,m_wordsConsumed(prevProcessedRule.m_wordsConsumed)
	{}
	ProcessedRule(const ProcessedRule &prevProcessedRule)
		:m_lastNode(prevProcessedRule.m_lastNode)
		,m_wordsConsumed(prevProcessedRule.m_wordsConsumed)
	{}
	ProcessedRule& operator=(const ProcessedRule &copy)
	{
		if(this != &copy)
		{
			m_lastNode			= copy.m_lastNode;
			m_wordsConsumed	= copy.m_wordsConsumed;
		}
		return *this;
	}

	const MosesOnDiskPt::SourcePhraseNode &GetLastNode() const
	{ return m_lastNode; }
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

const ChartRuleCollection *PhraseDictionaryOnDisk::GetChartRuleCollection(
																		InputType const& src
																		,WordsRange const& range
																		,bool adhereTableLimit) const
{
	const StaticData &staticData = StaticData::Instance();
	float weightWP = staticData.GetWeightWordPenalty();
	const LMList &lmList = staticData.GetAllLM();

	ChartRuleCollection *ret = new ChartRuleCollection();
	m_chartTargetPhraseColl.push_back(ret);

	// source phrase
	assert(src.GetType() == SentenceInput);
	Phrase *cachedSource = new Phrase(src.GetSubString(range));
	m_sourcePhrase.push_back(cachedSource);

	// non-terminal word
	FactorCollection &factorCollection = FactorCollection::Instance();

	assert(m_vocabLookup.find(NON_TERMINAL_FACTOR) != m_vocabLookup.end());
	MosesOnDiskPt::VocabId vocabIdNonTerm = m_vocabLookup.find(NON_TERMINAL_FACTOR)->second;
	MosesOnDiskPt::Word nonTermWord(m_inputFactorsVec.size(), vocabIdNonTerm);

	vector<	vector<ProcessedRule> > runningNodes(range.GetNumWordsCovered()+1);
	runningNodes[0].push_back(ProcessedRule(*m_initNode));

	// MAIN LOOP. create list of nodes of target phrases
	size_t relPos = 0;
	for (size_t absPos = range.GetStartPos(); absPos <= range.GetEndPos(); ++absPos)
	{
		vector<ProcessedRule>
					&todoNodes = runningNodes[relPos]
					,&doneNodes	= runningNodes[relPos+1];

		// convert to disk word
		const Word &origWord = src.GetWord(absPos);
		MosesOnDiskPt::Word searchWord(m_inputFactorsVec.size());
		bool validSearchWord = searchWord.ConvertFromMoses(m_inputFactorsVec, origWord, m_vocabLookup);

		vector<ProcessedRule>::iterator iterNode;
		for (iterNode = todoNodes.begin(); iterNode != todoNodes.end(); ++iterNode)
		{
			ProcessedRule &prevProcessedRule = *iterNode;
			const MosesOnDiskPt::SourcePhraseNode &todoNode = prevProcessedRule.GetLastNode();

			if (validSearchWord)
			{
				const MosesOnDiskPt::SourcePhraseNode *node =
								todoNode.GetChild(searchWord
																	,m_sourceFile
																	,m_inputFactorsVec);

				if (node != NULL)
				{
					ProcessedRule processedRule(prevProcessedRule, *node);
					processedRule.AddConsume(absPos, false);
					doneNodes.push_back(processedRule);
					delete node;
				}
			}

			// search for X (non terminals)
			const MosesOnDiskPt::SourcePhraseNode *node 
							= todoNode.GetChild(nonTermWord
																	,m_sourceFile
																	,m_inputFactorsVec);
			if (node != NULL)
			{
				ProcessedRule processedRule(prevProcessedRule, *node);
				processedRule.AddConsume(absPos, true);
				doneNodes.push_back(processedRule);
				delete node;
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
	} // main loop

	// return list of target phrases
	const size_t tableLimit = GetTableLimit();

	vector<ProcessedRule> &nodes = runningNodes.back();

	vector<ProcessedRule>::iterator iterNode;
	for (iterNode = nodes.begin(); iterNode != nodes.end(); ++iterNode)
	{
		// get target phrase coll from disk
		const MosesOnDiskPt::SourcePhraseNode &node = iterNode->GetLastNode();
		const vector<WordsConsumed> &wordsConsumed = iterNode->GetWordsConsumed();
		const MosesOnDiskPt::TargetPhraseList *targetPhraseCollectionDisk
							= node.GetTargetPhraseCollection(m_inputFactorsVec
																							,m_outputFactorsVec
																							,m_sourceFile
																							,m_targetFile
																							,GetNumScoreComponents());

		// populate real target phrase coll
		TargetPhraseCollection *targetPhraseCollection = new TargetPhraseCollection();

		MosesOnDiskPt::TargetPhraseList::const_iterator iter, iterEnd;

		for (iter = targetPhraseCollectionDisk->begin(); iter != targetPhraseCollectionDisk->end(); ++iter)
		{
			const MosesOnDiskPt::TargetPhrase &tpDisk = **iter;
			TargetPhrase *targetPhrase = tpDisk.ConvertToMoses(
																		m_outputFactorsVec
																		, m_targetLookup
																		, *this
																		, m_weight
																		, weightWP
																		, lmList
																		, *cachedSource
																		, wordsConsumed.size());
			targetPhraseCollection->Add(targetPhrase);
		}
		targetPhraseCollection->Prune(adhereTableLimit, tableLimit);

		ret->Add(*targetPhraseCollection, wordsConsumed, adhereTableLimit, tableLimit);

		m_cache.push_back(targetPhraseCollection);
		//delete targetPhraseCollection;

		delete targetPhraseCollectionDisk;
	} // convert runningNodes into target phrase

	return ret;
}

} // namespace

