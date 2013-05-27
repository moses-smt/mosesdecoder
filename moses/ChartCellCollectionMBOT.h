//Fabienne Braune
//Collection of cells for l-MBOT rules

//Comment on implementation : would have been cool to use the ChartCellCollectionBaseMBOT
//for containing also l-MBOT chart Cells (ChartCellMBOT)
//But then, the ChartCellCollectionBase constructor has to be modified
//in order to instantiate a ChartCellLabelMBOT instead of ChartCell

#pragma once

#include "InputType.h"
#include "ChartCellMBOT.h"
#include "ChartCellCollection.h"
#include "WordsRange.h"

#include <boost/ptr_container/ptr_vector.hpp>

namespace Moses
{
class InputType;
class ChartManager;

class ChartCellCollectionBaseMBOT : public ChartCellCollectionBase {
  public:
    template <class Factory> ChartCellCollectionBaseMBOT(const InputType &input, const Factory &factory)
     : ChartCellCollectionBase(input,factory)
     , m_cells(input.GetSize()) {

      std::cerr << "BUILDING CHART CELL COLLECTION BASE MBOT" << std::endl;
      size_t size = input.GetSize();
      for (size_t startPos = 0; startPos < size; ++startPos) {
        std::vector<ChartCellBaseMBOT*> &inner = m_cells[startPos];
        inner.reserve(size - startPos);
        for (size_t endPos = startPos; endPos < size; ++endPos) {
          inner.push_back(factory(startPos, endPos));
        }
        //Fabienne Braune : Would be cool to defer the instantiation outside of the constructor
        WordSequence firstWord;
        firstWord.Add(*(const_cast<Word*> (&(input.GetWord(startPos)))));
        m_source.push_back(new ChartCellLabelMBOT(inner[0]->GetCoverage(),firstWord));
      }
    }

    virtual ~ChartCellCollectionBaseMBOT();

    const ChartCellBaseMBOT &GetBaseMBOT(const WordsRange &coverage) const {
      return *m_cells[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
    }

    //Fabienne Braune : just in case GetBase gets somehow called
    const ChartCellBase &GetBase(const WordsRange &coverage) const {
    	std::cerr << "GetBase not implmented in ChartCellCollectionMBOT "<< std::endl;
       }

    ChartCellBaseMBOT &MutableBaseMBOT(const WordsRange &coverage) {
      return *m_cells[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
    }

    //Fabienne Braune : just in case MutableBase gets somehow called
     const ChartCellBase &MutableBase(const WordsRange &coverage) const {
       	std::cerr << "Mutable Base not implmented in ChartCellCollectionMBOT "<< std::endl;
    }

    const ChartCellLabelMBOT &GetSourceWordLabel(size_t at) const {
      return m_source[at];
    }

  private:
    std::vector<std::vector<ChartCellBaseMBOT*> > m_cells;
    boost::ptr_vector<ChartCellLabelMBOT> m_source;
};

/** Hold all the chart cells for 1 input sentence. A variable of this type is held by the ChartManager
 */
class ChartCellCollectionMBOT : public ChartCellCollectionBaseMBOT, public ChartCellCollection {
  public:
    ChartCellCollectionMBOT(const InputType &input, ChartManager &manager);

  //! get a chart cell for a particular range
  ChartCellMBOT &Get(const WordsRange &coverage) {
    return static_cast<ChartCellMBOT&>(MutableBaseMBOT(coverage));
  }

  //! get a chart cell for a particular range
  const ChartCellMBOT &Get(const WordsRange &coverage) const {
    return static_cast<const ChartCellMBOT&>(GetBaseMBOT(coverage));
  }
};

}

