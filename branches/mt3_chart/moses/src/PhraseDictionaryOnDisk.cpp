

#include "PhraseDictionaryOnDisk.h"
#include "InputFileStream.h"
#include "../../on-disk-phrase-dict/src/Phrase.h"
#include "../../on-disk-phrase-dict/src/TargetPhrase.h"
#include "../../on-disk-phrase-dict/src/SourcePhraseNode.h"
#include "../../on-disk-phrase-dict/src/TargetPhraseList.h"
#include "../../moses/src/FactorCollection.h"
#include "../../moses/src/StaticData.h"

using namespace std;

namespace Moses
{

PhraseDictionaryOnDisk::~PhraseDictionaryOnDisk()
{
	delete m_initNode;
}

bool PhraseDictionaryOnDisk::Load(const std::vector<FactorType> &input
							, const std::vector<FactorType> &output
							, const std::string &filePath
							, const std::vector<float> &weight
							, size_t tableLimit)
{
	m_tableLimit = tableLimit;
	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);
	m_inputFactorsVec		= input;
	m_outputFactorsVec	= output;

	m_weight = weight;

	// load vocab
	InputFileStream vocabFile(filePath + "/vocab.db");
	string line;
	//int lineNo = 0;

	while( !getline(vocabFile, line, '\n').eof())
	{
		vector<string> vecStr = Tokenize(line);
		assert(vecStr.size() == 2);
		MosesOnDiskPt::VocabId vocabId = Scan<MosesOnDiskPt::VocabId>(vecStr[1]);
		m_vocabLookup[ vecStr[0] ] = vocabId;
	}

	LoadTargetLookup();

	// assert #factors & #scores
	m_sourceFile.open((filePath + "/source.db").c_str(), ios::in|ios::binary|ios::ate);
	assert(m_sourceFile.is_open());

	m_sourceFile.seekg (0, ios::beg);

	// init offset
	long initOffset;
	m_sourceFile.read((char*) &initOffset, sizeof(initOffset));
	m_initNode = new MosesOnDiskPt::SourcePhraseNode(initOffset);

	// other variables
	size_t numFactors, numScores;

	m_sourceFile.read((char*) &numFactors, sizeof(numFactors));
	m_sourceFile.read((char*) &numScores, sizeof(numScores));

	assert(input.size() == numFactors);
	assert(GetNumScoreComponents() == numScores);

	m_targetFile.open((filePath + "/target.db").c_str(), ios::in|ios::binary|ios::ate);
	assert(m_targetFile.is_open());

	return true;
}

void PhraseDictionaryOnDisk::LoadTargetLookup()
{
	FactorCollection &factorCollection = FactorCollection::Instance();

	// vocabid starts from 1 !
	m_targetLookup.resize(m_vocabLookup.size() + 1);

	map<std::string, MosesOnDiskPt::VocabId>::iterator iter;
	for (iter = m_vocabLookup.begin(); iter != m_vocabLookup.end(); ++iter)
	{
		const string &str = iter->first;
		MosesOnDiskPt::VocabId vocabId = iter->second;

		std::vector<const Factor*> &entry = m_targetLookup[vocabId];
		entry.resize(m_outputFactorsVec.size());

		for (size_t ind = 0; ind < m_outputFactorsVec.size(); ++ind)
		{
			FactorType factorType = m_outputFactorsVec[ind];
			const Factor *factor = factorCollection.AddFactor(Moses::Output, factorType, str);
			entry[ind] = factor;
		}
	}
}

// PhraseDictionary impl
// for mert
void PhraseDictionaryOnDisk::SetWeightTransModel(const std::vector<float> &weightT)
{
}

//! find list of translations that can translates src. Only for phrase input
const TargetPhraseCollection *PhraseDictionaryOnDisk::GetTargetPhraseCollection(const Phrase& src) const
{
	const StaticData &staticData = StaticData::Instance();

	TargetPhraseCollection *ret = new TargetPhraseCollection();
	m_cache.push_back(ret);
	Phrase *cachedSource = new Phrase(src);
	m_sourcePhrase.push_back(cachedSource);

	const MosesOnDiskPt::SourcePhraseNode *nodeOld = new MosesOnDiskPt::SourcePhraseNode(*m_initNode);

	// find target phrases from tree
	size_t size = src.GetSize();
	for (size_t pos = 0; pos < size; ++pos)
	{
		// create on disk word from moses word
		const Word &origWord = src.GetWord(pos);
		MosesOnDiskPt::Word searchWord(m_inputFactorsVec.size());

		const MosesOnDiskPt::SourcePhraseNode *nodeNew;

		bool success = searchWord.ConvertFromMoses(m_inputFactorsVec, origWord, m_vocabLookup);
		if (!success)
		{
			nodeNew = NULL;
		}
		else
		{	// search for word in node map
			nodeNew = nodeOld->GetChild(searchWord, m_sourceFile, m_inputFactorsVec);
		}

		delete nodeOld;
		nodeOld = nodeNew;

		if (nodeNew == NULL)
		{ // nothing found. end
			break;
		}
	} // for (size_t pos

	if (nodeOld)
	{ // found node. create target phrase collection
		const MosesOnDiskPt::TargetPhraseList *diskList = nodeOld->GetTargetPhraseCollection(
																						m_inputFactorsVec
																						, m_outputFactorsVec
																						, m_sourceFile
																						, m_targetFile
																						, GetNumScoreComponents());

		MosesOnDiskPt::TargetPhraseList::const_iterator iter;
		for (iter = diskList->begin(); iter != diskList->end(); ++iter)
		{
			const MosesOnDiskPt::TargetPhrase &tpDisk = **iter;

			float weightWP = staticData.GetWeightWordPenalty();
			const LMList &lmList = staticData.GetAllLM();
			TargetPhrase *targetPhrase = tpDisk.ConvertToMoses(
																		m_outputFactorsVec
																		, m_targetLookup
																		, *this
																		, m_weight
																		, weightWP
																		, lmList
																		, *cachedSource
																		, size);

			ret->Add(targetPhrase);
		}

		delete diskList;
	}

	delete nodeOld;

	return ret;
}

long PhraseDictionaryOnDisk::SearchNode(long offset, const MosesOnDiskPt::Word &searchWord) const
{
	m_sourceFile.seekg(offset, ios::beg);

	size_t mapSize;
	m_sourceFile.read((char*) &mapSize, sizeof(mapSize));

	for (size_t ind = 0; ind < mapSize; ++ind)
	{
		MosesOnDiskPt::Word foundWord(m_inputFactorsVec.size());
		foundWord.Load(m_sourceFile, m_inputFactorsVec);

		long nextNodeOffset;
		m_sourceFile.read((char*) &nextNodeOffset, sizeof(nextNodeOffset));

		if (foundWord > searchWord)
		{ // already gone past without finding word. Not there
			return 0;
		}

		if (foundWord < searchWord)
		{ // haven't reached it. Might still be there. Move onto next word on disk
			continue;
		}

		// found
		assert(foundWord == searchWord);
		return nextNodeOffset;
	}

	// end. nothing found
	return 0;
}

//! Create entry for translation of source to targetPhrase
void PhraseDictionaryOnDisk::AddEquivPhrase(const Phrase &source, TargetPhrase *targetPhrase)
{
}

void PhraseDictionaryOnDisk::CleanUp()
{
	RemoveAllInColl(m_cache);
	RemoveAllInColl(m_sourcePhrase);
	RemoveAllInColl(m_chartTargetPhraseColl);
}

} //namespace Moses


