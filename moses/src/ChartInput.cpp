
#include "ChartInput.h"
#include "StaticData.h"
#include "Util.h"

using namespace std;

namespace Moses
{

//! populate this InputType with data from in stream
int ChartInput::Read(std::istream& in,const std::vector<FactorType>& factorOrder)
{
	const StaticData &staticData = StaticData::Instance();

	const string& factorDelimiter = StaticData::Instance().GetFactorDelimiter();
	string line;

	if (getline(in, line, '\n').eof())	
			return 0;
	// remove extra spaces
	line = Trim(line);

	// contains 2 parts, the normal sentence, and chart info
	vector<string> vecSurfaceAndParse = TokenizeMultiCharSeparator(line, "|||");

	// normal sentence
	if (staticData.GetSearchAlgorithm() == ChartDecoding)
	{
		Word &start = AddWord();
		InitStartEndWord(start, true);
	}

	Phrase::CreateFromString(factorOrder, vecSurfaceAndParse[0], factorDelimiter);

	if (staticData.GetSearchAlgorithm() == ChartDecoding)
	{
		Word &end = AddWord();
		InitStartEndWord(end, false);
	}

	// size input chart
	size_t sourceSize = GetSize();
	m_sourceChart.resize(sourceSize);

	for (size_t pos = 0; pos < sourceSize; ++pos)
	{
		m_sourceChart[pos].resize(sourceSize - pos);
	}

	// chart info
	if 	(vecSurfaceAndParse.size() == 1) 
	{
		string label = staticData.GetDefaultNonTerminal();
		for (size_t startPos = 0; startPos < sourceSize; ++startPos)
		{
			for (size_t endPos = startPos; endPos < sourceSize; ++endPos)
			{
				AddChartLabel(startPos, endPos, label, factorOrder);
			}
		}
	}
	else
	{
		assert(vecSurfaceAndParse.size() == 2);
		vector<string> vecParse = Tokenize(vecSurfaceAndParse[1]);
		vector<string>::const_iterator iterParse;
		for (iterParse = vecParse.begin(); iterParse != vecParse.end(); ++iterParse)
		{
			const string &entry = *iterParse;
			vector<string> vecEntry = Tokenize(entry, "=");
			assert(vecEntry.size() == 2);
			
			if (vecEntry[1].substr(0,1) == "<")
				continue;
			
			string label = Tokenize<string>(vecEntry[1], "-")[0];
			
			string span = vecEntry[0].substr(1, vecEntry[0].size() - 2);
			vector<size_t> vecSpan = Tokenize<size_t>(span, "-,");
			assert(vecSpan.size() == 2);
			
			AddChartLabel(vecSpan[0] + 1, vecSpan[1] + 1, label, factorOrder);
		}

		string label = staticData.GetDefaultNonTerminal();
		for (size_t startPos = 0; startPos < sourceSize; ++startPos)
		{
			for (size_t endPos = startPos; endPos < sourceSize; ++endPos)
			{
				AddChartLabel(startPos, endPos, label, factorOrder);
			}
		}

	}

	return 1;
}

//! Output debugging info to stream out
void ChartInput::Print(std::ostream &out) const
{
	out << *this << "\n";
}

//! create trans options specific to this InputType
TranslationOptionCollection* ChartInput::CreateTranslationOptionCollection() const
{

	return NULL;
}

void ChartInput::AddChartLabel(size_t startPos, size_t endPos, const string &label
															, const std::vector<FactorType>& factorOrder)
{
	Word word;
	
	// TODO
	// to many ways to specify non-terms. need to rationalise
	//word.CreateFromString(Input, factorOrder, label);
	
	const Factor *factor = FactorCollection::Instance().AddFactor(Input, factorOrder[0], label, true);
	word.SetFactor(0, factor);
	word.SetIsNonTerminal(true);
	
	SourceLabelOverlap overlapType = StaticData::Instance().GetSourceLabelOverlap();
	LabelList &list = GetLabelList(startPos, endPos);
	switch (overlapType)
	{
		case SourceLabelOverlapAdd: 
			list.push_back(word);
			break;
		case SourceLabelOverlapReplace:
			if (list.size() > 0) // replace existing label
				list[0] = word;
			else
				list.push_back(word);
			break;
		case SourceLabelOverlapDiscard:
			if (list.size() == 0) 
				list.push_back(word);
			break;
	}
}

std::ostream& operator<<(std::ostream &out, const ChartInput &input)
{	
	out<< static_cast<Phrase const&>(input) << " ||| ";
	
	size_t size = input.GetSize();
	for (size_t startPos = 0; startPos < size; ++startPos)
	{
		for (size_t endPos = startPos; endPos < size; ++endPos)
		{
			const LabelList &labelList = input.GetLabelList(startPos, endPos);
			LabelList::const_iterator iter;
			for (iter = labelList.begin(); iter != labelList.end(); ++iter)
			{
				const Word &word = *iter;
				out << "[" << startPos <<"," << endPos << "]=" 
						<< word << "(" << word.IsNonTerminal() << ") ";
				assert(word.IsNonTerminal());
			}
		}		
	}

	return out;
}

	
} // namespace

