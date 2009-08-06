
#pragma once

#include <vector>
#include "Sentence.h"

namespace Moses
{
class ChartInput : public Sentence
{
	friend std::ostream& operator<<(std::ostream&, const ChartInput&);

protected:
	std::vector<std::vector<LabelList> > m_sourceChart;

	void AddChartLabel(size_t startPos, size_t endPos, const std::string &label
										,const std::vector<FactorType>& factorOrder);
	LabelList &GetLabelList(size_t startPos, size_t endPos)
	{
		return m_sourceChart[startPos][endPos - startPos];
	}

public:
	ChartInput(FactorDirection direction)	
		: Sentence(direction)
	{}

	InputTypeEnum GetType() const
	{	return ChartSentenceInput;}

	//! populate this InputType with data from in stream
	virtual int Read(std::istream& in,const std::vector<FactorType>& factorOrder);
	
	//! Output debugging info to stream out
	virtual void Print(std::ostream&) const;

	//! create trans options specific to this InputType
	virtual TranslationOptionCollection* CreateTranslationOptionCollection() const;

	virtual const LabelList &GetLabelList(size_t startPos, size_t endPos) const
	{	return m_sourceChart[startPos][endPos - startPos];	}

};

}

