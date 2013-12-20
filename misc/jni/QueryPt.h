// Query binary phrase tables.
// Marcin Junczys-Dowmunt, 13 September 2012

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#define HAVE_CMPH
#define WITH_THREADS

namespace Moses {
  class PhraseDictionaryFeature;
  class PhraseDictionaryCompact;
  class LMList;
}

class QueryPt {
  private:
    int m_nscores;
    bool m_useAlignments;
    bool m_reportCounts;
    std::vector<size_t> m_input;
    std::vector<size_t> m_output;
    std::vector<float>  m_weight;
  
    boost::shared_ptr<Moses::PhraseDictionaryFeature> m_pdf;
    boost::shared_ptr<Moses::PhraseDictionaryCompact> m_pdc;
    boost::shared_ptr<Moses::LMList> m_lmList;
  
  public:
    QueryPt(const std::string& ttable);
    std::string query(std::string phrase);
    void usage();
};

