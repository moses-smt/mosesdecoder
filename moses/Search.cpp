#include "Manager.h"
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "SearchNormalBatch.h"
#include "util/exception.hh"

namespace Moses
{

Search::Search(Manager& manager)
  : m_manager(manager)
  ,m_inputPath()
  ,m_initialTransOpt()
{
  m_initialTransOpt.SetInputPath(m_inputPath);
}


Search *Search::CreateSearch(Manager& manager, const InputType &source,
                             SearchAlgorithm searchAlgorithm, const TranslationOptionCollection &transOptColl)
{
  switch(searchAlgorithm) {
  case Normal:
    return new SearchNormal(manager,source, transOptColl);
  case CubePruning:
    return new SearchCubePruning(manager, source, transOptColl);
  case NormalBatch:
    return new SearchNormalBatch(manager, source, transOptColl);
  default:
    UTIL_THROW2("ERROR: search. Aborting\n");
    return NULL;
  }
}

}
