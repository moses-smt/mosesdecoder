#ifndef moses_InputTreeRep_h
#define moses_InputTreeRep_h

#include <vector>
#include <vector>
#include <string>
#include "InputType.h"

namespace Moses
{

//! @todo what is this?
class XMLParseOutputForTrain
{
public:
  std::string m_label;
  WordsRange m_range;

  XMLParseOutputForTrain(const std::string &label, const WordsRange &range)
    : m_label(label)
    , m_range(range)
  {}
};

/**
Reimplementation of InputTree for extracting syntax features during training
**/

class SyntaxLabel
{
	public:
	std::string m_label;
	bool m_nonTerm;
	SyntaxLabel(const std::string &label, bool isNonTerm);

	const std::string GetString() const
	{
		return m_label;
	}

	const bool IsNonTerm() const
	{
		return m_nonTerm;
	}
};

class InputTreeRep
{

typedef std::vector<SyntaxLabel> SyntLabels;

protected:
  std::vector<std::vector<SyntLabels> > m_sourceChart;
  std::vector<std::string> m_sourceSentence;

  void AddChartLabel(size_t startPos, size_t endPos, const SyntaxLabel &label);
  SyntLabels &GetLabels(size_t startPos, size_t endPos) {
    return m_sourceChart[startPos][endPos - startPos];
  }

  bool ProcessAndStripXMLTags(std::string &line, std::vector<XMLParseOutputForTrain> &sourceLabelss);

public:
  InputTreeRep()
  {}

  InputTypeEnum GetType() const {
    return TreeInputType;
  }

  std::vector<std::string> GetSourceSentence()
  {
      return m_sourceSentence;
  }

  size_t GetSourceSentenceSize()
  {
        return m_sourceSentence.size();
  }

  //! populate this InputType with data from in stream
  int Read(std::istream& in);

  //! Output debugging info to stream out
  void Print(std::ostream&);
};

}

#endif
