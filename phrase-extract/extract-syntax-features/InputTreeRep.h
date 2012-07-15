#ifndef moses_InputTreeRep_h
#define moses_InputTreeRep_h

#include <vector>
#include <vector>
#include <string>
#include "Word.h"
#include "InputType.h"

namespace Moses
{

typedef std::vector<Word> SyntLabels;

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


class InputTreeRep
{

protected:
  std::vector<std::vector<SyntLabels> > m_sourceChart;
  std::vector<std::string> m_sourceSentence;

  void AddChartLabel(size_t startPos, size_t endPos, const std::string &label
                     ,const std::vector<FactorType>& factorOrder);
  void AddChartLabel(size_t startPos, size_t endPos, const Word &label
                     ,const std::vector<FactorType>& factorOrder);
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
  int Read(std::istream& in,const std::vector<FactorType>& factorOrder);

  //! Output debugging info to stream out
  void Print(std::ostream&);
};

}

#endif
