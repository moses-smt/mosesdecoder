#include "Manager.h"
#include "SearchCubePruning.h"
#include "SearchNormal.h"
#include "InputType.h"
#include "util/exception.hh"

namespace Moses
{

Search::Search(Manager& manager)
  : m_manager(manager)
  , m_source(manager.GetSource())
  , m_options(*manager.options())
  , m_inputPath()
  , m_initialTransOpt()
  , m_bitmaps(manager.GetSource().GetSize(), manager.GetSource().m_sourceCompleted)
  , interrupted_flag(0)
{
  m_initialTransOpt.SetInputPath(m_inputPath);
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
