#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include "PhraseDictionaryJoshua.h"
#include "InputType.h"
#include "InputFileStream.h"
#include "StaticData.h"

using namespace std;

namespace Moses
{

PhraseDictionaryJoshua::~PhraseDictionaryJoshua()
{

}

inline void TransformString(vector< vector<string> > &phraseVector
									 ,vector<pair<size_t, size_t> > &alignVector)
{
	assert(alignVector.size() == 0);

	// source
	for (size_t pos = 0 ; pos < phraseVector.size() ; ++pos)
	{
		string &str = phraseVector[pos][0];
		if (str.size() > 3 && str.substr(0, 3) == "[X,")
		{ // non-term
			string indStr = str.substr(3, str.size() - 4);
			size_t indAlign = Scan<size_t>(indStr) - 1;

			pair<size_t, size_t> alignPair(pos, indAlign);
			alignVector.push_back(alignPair);

			str = "[X]";
		}
	}
}

inline void TransformString(vector< vector<string> > &sourcePhraseVector
										 ,vector< vector<string> > &targetPhraseVector
										 ,string &sourceAlign
										 ,string &targetAlign)
{
	vector<string>  sourceAlignVec(sourcePhraseVector.size(), "()");
	vector<string>  targetAlignVec(targetPhraseVector.size(), "()");

	vector<pair<size_t, size_t> > sourceNonTerm, targetNonTerm;

	TransformString(sourcePhraseVector, sourceNonTerm);
	TransformString(targetPhraseVector, targetNonTerm);

	for (size_t ind = 0 ; ind < sourceNonTerm.size() ; ++ind)
	{
		size_t sourcePos	= sourceNonTerm[ind].first
					,targetInd	= sourceNonTerm[ind].second;
		size_t targetPos	= targetNonTerm[targetInd].first;

		sourceAlignVec[sourcePos] = "(" + SPrint(targetPos) + ")";
		targetAlignVec[targetPos] = "(" + SPrint(sourcePos) + ")";
	}

	// concate string
	stringstream strme("");
	for (size_t ind = 0 ; ind < sourceAlignVec.size() ; ++ind)
		strme << sourceAlignVec[ind] << " ";
	sourceAlign = strme.str();

	strme.str("");
	for (size_t ind = 0 ; ind < targetAlignVec.size() ; ++ind)
		strme << targetAlignVec[ind] << " ";
	targetAlign = strme.str();
}


bool PhraseDictionaryJoshua::Load(const std::vector<FactorType> &input
								, const std::vector<FactorType> &output
								, const std::string &joshuaPath
								, const std::string &sourcePath
								, const std::string &targetPath
								, const std::string &alignPath
								, const std::vector<float> &weight
								, float weightWP
								, size_t tableLimit)
{
	m_tableLimit = tableLimit;

	//factors
	m_inputFactorsVec = input;
	m_outputFactorsVec = output;

	m_inputFactors = FactorMask(input);
	m_outputFactors = FactorMask(output);

	m_weight = weight;
	m_weightWP = weightWP;

	m_joshuaPath	= joshuaPath;
	m_sourcePath	= sourcePath;
	m_targetPath	= targetPath;
	m_alignPath		= alignPath;

	return true;
}

void PhraseDictionaryJoshua::InitializeForInput(InputType const &source)
{
	assert(source.GetType() == SentenceInput || source.GetType() == ChartSentenceInput);

	// temp file with current sentence
	std::ofstream sourceSentenceFileStrme;
	string sourceSentenceFilePath;
	CreateTempFile(sourceSentenceFileStrme, sourceSentenceFilePath);

	// write out only factors needed by pt
	for (size_t pos = 0; pos < source.GetSize(); ++pos)
	{
		const Word &word = source.GetWord(pos);

		// 0th factor
		const Factor *factor = word.GetFactor(m_inputFactorsVec[0]);
		sourceSentenceFileStrme << factor->GetString();
		for (size_t factorInd = 1; factorInd < m_inputFactorsVec.size(); ++factorInd)
		{
			factor = word.GetFactor(m_inputFactorsVec[factorInd]);
			sourceSentenceFileStrme << "|" << factor->GetString();
		}
		sourceSentenceFileStrme << " ";
	}
	sourceSentenceFileStrme << endl;
	sourceSentenceFileStrme.close();

	// filtered output file
	std::ofstream filteredFileStrme;
	string filteredFilePath;
	CreateTempFile(filteredFileStrme, filteredFilePath);
	filteredFileStrme.close();

	// call java
	stringstream strme;
	strme << string("java ") << StaticData::Instance().GetJavaArgs() << " -cp \""
				<< m_joshuaPath
				<< "\" joshua.sarray.ExtractRules"
				<< " --source=" + m_sourcePath
				<< " --target=" + m_targetPath
				<< " --alignments=" + m_alignPath
				<< " --test=" << sourceSentenceFilePath
				<< " --maxPhraseLength=5"
				<< " > " << filteredFilePath;
	cerr << strme.str() << endl;

	system(strme.str().c_str());

	DeleteFile(sourceSentenceFilePath);

	// read in filtered pt as normal
	assert(FileExists(filteredFilePath));
	assert(m_collection.GetSize() == 0);
	const StaticData &staticData = StaticData::Instance();
	const LMList &languageModels = staticData.GetAllLM();

	InputFileStream inFile(filteredFilePath);
	MyBase::Load(m_inputFactorsVec, m_outputFactorsVec
			, inFile, m_weight, m_tableLimit
			, languageModels, m_weightWP);

	DeleteFile(filteredFilePath);

	if (m_collection.GetSize() == 0)
	{ // nothing loaded. the java extractor is probably wrong
		TRACE_ERR("ERROR: No phrasees from Joshua pt");
		assert(false);
	}
		
	MyBase::InitializeForInput(source);
}

void PhraseDictionaryJoshua::SetWeightTransModel(const std::vector<float> &weightT)
{
	MyBase::SetWeightTransModel(weightT);
}

void PhraseDictionaryJoshua::CleanUp()
{
	m_collection.CleanUp();
	MyBase::CleanUp();
}

const TargetPhraseCollection *PhraseDictionaryJoshua::GetTargetPhraseCollection(const Phrase& source) const
{
	return MyBase::GetTargetPhraseCollection(source);
}

TargetPhraseCollection &PhraseDictionaryJoshua::GetOrCreateTargetPhraseCollection(const Phrase &source)
{
	return MyBase::GetOrCreateTargetPhraseCollection(source);
}

const ChartRuleCollection *PhraseDictionaryJoshua::GetChartRuleCollection(
																				InputType const& src
																				,WordsRange const& range
																				,bool adhereTableLimit
																				,const CellCollection &cellColl) const
{
	return MyBase::GetChartRuleCollection(src, range, adhereTableLimit, cellColl);
}


} // namespace

