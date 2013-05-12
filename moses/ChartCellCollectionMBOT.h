//Fabienne Braune
//Collection of cells for l-MBOT rules

//Comment on implementation : would have been cool to use the ChartCellCollectionBaseMBOT
//for containing also l-MBOT chart Cells (ChartCellMBOT)
//But then, the ChartCellCollectionBase constructor has to be modified
//in order to instantiate a ChartCellLabelMBOT instead of ChartCell

#pragma once

#include "InputType.h"
#include "ChartCell.h"
#include "WordsRange.h"

#include <boost/ptr_container/ptr_vector.hpp>

namespace Moses
{
class InputType;
class ChartManager;

class ChartCellCollectionBaseMBOT {
  public:
    template <class Factory> ChartCellCollectionBaseMBOT(const InputType &input, const Factory &factory) :
      m_cells(input.GetSize()) {
      size_t size = input.GetSize();
      for (size_t startPos = 0; startPos < size; ++startPos) {
        std::vector<ChartCellBase*> &inner = m_cells[startPos];
        inner.reserve(size - startPos);
        for (size_t endPos = startPos; endPos < size; ++endPos) {
          inner.push_back(factory(startPos, endPos));
        }
        //Fabienne Braune : Would be cool to defer the instantiation outside of the constructor
        m_source.push_back(new ChartCellLabel(inner[0]->GetCoverage(), input.GetWord(startPos)));
      }
    }

    virtual ~ChartCellCollectionBaseMBOT();

    const ChartCellBase &GetBase(const WordsRange &coverage) const {
      return *m_cells[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
    }

    ChartCellBase &MutableBase(const WordsRange &coverage) {
      return *m_cells[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()];
    }


    const ChartCellLabel &GetSourceWordLabel(size_t at) const {
      return m_source[at];
    }

  private:
    std::vector<std::vector<ChartCellBase*> > m_cells;

    boost::ptr_vector<ChartCellLabel> m_source;
};

/** Hold all the chart cells for 1 input sentence. A variable of this type is held by the ChartManager
 */
class ChartCellCollection : public ChartCellCollectionBase {
  public:
    ChartCellCollection(const InputType &input, ChartManager &manager);

  //! get a chart cell for a particular range
  ChartCell &Get(const WordsRange &coverage) {
    return static_cast<ChartCell&>(MutableBase(coverage));
  }

  //! get a chart cell for a particular range
  const ChartCell &Get(const WordsRange &coverage) const {
    return static_cast<const ChartCell&>(GetBase(coverage));
  }
};

}

