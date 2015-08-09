#include "Manager.h"
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "util/exception.hh"

namespace Moses
{

Search::Search(Manager& manager)
  : m_manager(manager)
  , m_inputPath()
  , m_initialTransOpt()
  , m_options(manager.options())
  , interrupted_flag(0)
{
  m_initialTransOpt.SetInputPath(m_inputPath);
}


Search *
Search::
CreateSearch(Manager& manager, const InputType &source,
             SearchAlgorithm searchAlgorithm,
             const TranslationOptionCollection &transOptColl)
{
  switch(searchAlgorithm) {
  case Normal:
    return new SearchNormal(manager,source, transOptColl);
  case CubePruning:
    return new SearchCubePruning(manager, source, transOptColl);
  default:
    UTIL_THROW2("ERROR: search. Aborting\n");
    return NULL;
  }
}

bool
Search::
out_of_time()
{
  int const& timelimit = m_options.search.timeout;
  if (!timelimit) return false;
  double elapsed_time = GetUserTime();
  if (elapsed_time <= timelimit) return false;
  VERBOSE(1,"Decoding is out of time (" << elapsed_time << ","
          << timelimit << ")" << std::endl);
  interrupted_flag = 1;
  return true;
}

}
