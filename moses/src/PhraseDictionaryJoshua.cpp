
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
	assert(source.GetType() == SentenceInput);

	const StaticData &staticData = StaticData::Instance();
	const LMList &languageModels = staticData.GetAllLM();

	std::ofstream tempFile("temp.txt");
	tempFile << source;
	tempFile.close();

	stringstream strme;
	strme << string("java -Xmx512m -cp \"") 
				<< m_joshuaPath 
				<< "\" joshua.sarray.ExtractRules"
				<< " --source=" + m_sourcePath
				<< " --target=" + m_targetPath
				<< " --alignments=" + m_alignPath
				<< " --test=temp.txt" 
				<< " --maxPhraseLength=5"
				<< " > filtered.txt";

	system(strme.str().c_str());

	assert(FileExists("filtered.txt"));
	
	InputFileStream tempStream("filtered.txt");

	size_t count = 0;
	string line;
	while(getline(tempStream, line))
	{
		vector<string> tokens = TokenizeMultiCharSeparator( line , "|||" );
		assert(tokens.size() == 4);

		assert(Trim(tokens[0]) == "[X]");
		string sourcePhraseString	=tokens[1]
					, targetPhraseString=tokens[2]
					, scoreString				=tokens[3];

		bool isLHSEmpty = (sourcePhraseString.find_first_not_of(" \t", 0) == string::npos);
		if (isLHSEmpty && !staticData.IsWordDeletionEnabled()) {
			TRACE_ERR( "line " << count << ": pt entry contains empty target, skipping\n");
			continue;
		}

		vector<float> scoreVector = Tokenize<float>(scoreString);
		if (scoreVector.size() != m_numScoreComponent)
		{
			stringstream strme;
			strme << "Size of scoreVector != number (" <<scoreVector.size() << "!=" <<m_numScoreComponent<<") of score components on line " << count;
			UserMessage::Add(strme.str());
			abort();
		}
		assert(scoreVector.size() == m_numScoreComponent);

		const std::string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
		vector< vector<string> >	sourcePhraseVector = Phrase::Parse(sourcePhraseString, m_inputFactorsVec, factorDelimiter)
															,targetPhraseVector = Phrase::Parse(targetPhraseString, m_outputFactorsVec, factorDelimiter);
		string sourceAlign, targetAlign;
		TransformString(sourcePhraseVector, targetPhraseVector, sourceAlign, targetAlign);

		// source
		Phrase sourcePhrase(Input);
		sourcePhrase.CreateFromString( m_inputFactorsVec, sourcePhraseVector);

		//target
		TargetPhrase targetPhrase(Output);
		targetPhrase.SetSourcePhrase(&sourcePhrase);
		targetPhrase.CreateFromString( m_outputFactorsVec, targetPhraseVector);

		targetPhrase.CreateAlignmentInfo(sourceAlign, targetAlign);

		// component score, for n-best output
		std::vector<float> scv(scoreVector.size());
		std::transform(scoreVector.begin(),scoreVector.end(),scv.begin(),NegateScore);

		std::transform(scv.begin(),scv.end(),scv.begin(),FloorScore);
		targetPhrase.SetScore(this, scv, m_weight, m_weightWP, languageModels);

		AddEquivPhrase(sourcePhrase, targetPhrase);

		count++;
	}

}
	
void PhraseDictionaryJoshua::SetWeightTransModel(const std::vector<float> &weightT)
{
	MyBase::SetWeightTransModel(weightT);
}

void PhraseDictionaryJoshua::CleanUp()
{
	m_collection.CleanUp();
}

const TargetPhraseCollection *PhraseDictionaryJoshua::GetTargetPhraseCollection(const Phrase& source) const
{
	return MyBase::GetTargetPhraseCollection(source);
}

void PhraseDictionaryJoshua::AddEquivPhrase(const Phrase &source, const TargetPhrase &targetPhrase)
{
	MyBase::AddEquivPhrase(source, targetPhrase);
}

TargetPhraseCollection *PhraseDictionaryJoshua::CreateTargetPhraseCollection(const Phrase &source)
{
	return MyBase::CreateTargetPhraseCollection(source);
}

const ChartRuleCollection *PhraseDictionaryJoshua::GetChartRuleCollection(
																				InputType const& src
																				,WordsRange const& range
																				,bool adhereTableLimit) const
{
	return MyBase::GetChartRuleCollection(src, range, adhereTableLimit);
}


} // namespace

