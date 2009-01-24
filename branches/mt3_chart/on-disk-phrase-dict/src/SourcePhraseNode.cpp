
#include "SourcePhraseNode.h"
#include "Phrase.h"
#include "Word.h"
#include "TargetPhrase.h"
#include "TargetPhraseCollection.h"
#include "TargetPhraseList.h"

using namespace std;

namespace MosesOnDiskPt
{

SourcePhraseNode &SourcePhraseNode::AddWord(const Word &word)
{
	NodeMap::iterator iter = m_map.find(word);
	if (iter != m_map.end())
	{
		return iter->second;
	}
	else
	{
		SourcePhraseNode &node = m_map[word];
		return node;
	}
}

void SourcePhraseNode::AddTarget(const Phrase &targetPhrase
								,const std::vector<float> &scores
								,const std::vector< std::pair<size_t, size_t> > &align)
{
	TargetPhrase *storePhrase = new TargetPhrase(targetPhrase, scores, align);
	m_targetPhrases.push_back(storePhrase);
}

void SourcePhraseNode::Save(std::ostream &outStream)
{
	// write children 1st
	NodeMap::iterator iterMap;
	for (iterMap = m_map.begin() ; iterMap != m_map.end() ; ++iterMap)
	{
		SourcePhraseNode &node = iterMap->second;
		node.Save(outStream);
	}

	// write nodes in this map
	m_binOffset = outStream.tellp();

	size_t mapSize = m_map.size();
	outStream.write((char*) &mapSize, sizeof(mapSize));

	for (iterMap = m_map.begin() ; iterMap != m_map.end() ; ++iterMap)
	{
		const Word &word = iterMap->first;
		SourcePhraseNode &node = iterMap->second;

		word.Save(outStream);

		long offset = node.GetBinOffset();
		outStream.write((char*) &offset, sizeof(offset));
	}

	// num target phrases in this node
	size_t numTargetPhrases = m_targetPhrases.size();
	outStream.write((char*) &numTargetPhrases, sizeof(numTargetPhrases));

	for (size_t ind = 0; ind < m_targetPhrases.size(); ++ind)
	{
		TargetPhrase &targetPhrase = *m_targetPhrases[ind];
		targetPhrase.Save(outStream);
	}
}

const SourcePhraseNode *SourcePhraseNode::GetChild(const Word &searchWord
																									,std::ifstream &sourceFile
																									,const std::vector<Moses::FactorType> &inputFactorsVec
																									,size_t mapSize
																									,long startOffset) const
{
	const size_t memSize = searchWord.GetDiskSize() // size of word
													+ sizeof(long); // next pointer
	size_t numInputFactors = inputFactorsVec.size();

	int first = 0
			,last = mapSize - 1;

	while (first <= last)
	{
		int mid = (first + last) / 2;  // compute mid point.

		long temp = sourceFile.tellg();
		sourceFile.seekg(startOffset + mid * memSize, ios::beg);
		temp = sourceFile.tellg();

		MosesOnDiskPt::Word foundWord(numInputFactors);
		foundWord.Load(sourceFile, inputFactorsVec);

		if (searchWord > foundWord)
			first = mid + 1;  // repeat search in top half.
		else if (searchWord < foundWord)
			last = mid - 1; // repeat search in bottom half.
		else
		{	// found
			assert(foundWord == searchWord);

			long nextNodeOffset;
			sourceFile.read((char*) &nextNodeOffset, sizeof(nextNodeOffset));
			return new SourcePhraseNode(nextNodeOffset);
		}
	}

	return NULL; // failed to find key
}

const SourcePhraseNode *SourcePhraseNode::GetChild(const Word &searchWord
																									,std::ifstream &sourceFile
																									,const std::vector<Moses::FactorType> &inputFactorsVec) const
{
	assert(inputFactorsVec.size() == searchWord.GetSize());

	sourceFile.seekg(m_binOffset, ios::beg);

	size_t mapSize;
	sourceFile.read((char*) &mapSize, sizeof(mapSize));

	long startOffset = sourceFile.tellg();
	return GetChild(searchWord, sourceFile, inputFactorsVec, mapSize, startOffset);
}

const TargetPhraseList *SourcePhraseNode::GetTargetPhraseCollection(
																				const std::vector<Moses::FactorType> &inputFactorsVec
																				,const std::vector<Moses::FactorType> &outputFactorsVec
																				,std::ifstream &sourceFile
																				,std::ifstream &targetFile
																				,size_t numScores) const
{
	TargetPhraseList *ret = new TargetPhraseList();

	sourceFile.seekg(m_binOffset, ios::beg);
	size_t mapSize; // number of node coming out of here
	sourceFile.read((char*) &mapSize, sizeof(mapSize));

	// start of target phrase coll
	long jump = sourceFile.tellg();
	jump			+= mapSize * (inputFactorsVec.size() * sizeof(MosesOnDiskPt::VocabId) + sizeof(long));
	sourceFile.seekg(jump, ios::beg);

	size_t numTargetPhrases;
	sourceFile.read((char*) &numTargetPhrases, sizeof(numTargetPhrases));

	for (size_t ind = 0; ind < numTargetPhrases; ++ind)
	{
		long phraseOffset;
		sourceFile.read((char*) &phraseOffset, sizeof(phraseOffset));
		targetFile.seekg(phraseOffset, ios::beg);

		MosesOnDiskPt::Phrase *phrase = new MosesOnDiskPt::Phrase();
		phrase->Load(targetFile, outputFactorsVec);

		MosesOnDiskPt::TargetPhrase *targetPhrase
								= new MosesOnDiskPt::TargetPhrase(*phrase);
		targetPhrase->Load(sourceFile, numScores);

		ret->Add(phrase, targetPhrase);
	}

	return ret;
}

} // namespace



