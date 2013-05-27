
#include "ChartCellCollectionMBOT.h"
#include "ChartCellCollection.h"
#include "InputType.h"
#include "WordsRange.h"

namespace Moses {

ChartCellCollectionBaseMBOT::~ChartCellCollectionBaseMBOT() {
  m_source.clear();
  for (std::vector<std::vector<ChartCellBaseMBOT*> >::iterator i = m_cells.begin(); i != m_cells.end(); ++i)
    RemoveAllInColl(*i);
}

//Fabienne Braune : Had to rewrite the factory for instantiating l-MBOT chart cells
class CubeCellMBOTFactory {
  public:
    explicit CubeCellMBOTFactory(ChartManager &manager) : m_manager(manager) {}

    ChartCellMBOT *operator()(size_t start, size_t end) const {
      return new ChartCellMBOT(start, end, m_manager);
    }

  private:
    ChartManager &m_manager;
};

/** Costructor
 \param input the input sentence
 \param manager reference back to the manager
 */
ChartCellCollectionMBOT::ChartCellCollectionMBOT(const InputType &input, ChartManager &manager)
  : ChartCellCollection(input,manager)
  ,ChartCellCollectionBaseMBOT(input, CubeCellMBOTFactory(manager)) {}

} // namespace

