#include "TranslationOptionList.h"
#include "Util.h"
#include "TranslationOption.h"
#include <boost/foreach.hpp>

using namespace std;

namespace Moses
{

TranslationOptionList::
TranslationOptionList(const TranslationOptionList &copy)
{
  const_iterator iter;
  for (iter = copy.begin(); iter != copy.end(); ++iter) {
    const TranslationOption &origTransOpt = **iter;
    TranslationOption *newTransOpt = new TranslationOption(origTransOpt);
    Add(newTransOpt);
  }
}

TranslationOptionList::
~TranslationOptionList()
{
  RemoveAllInColl(m_coll);
}

TO_STRING_BODY(TranslationOptionList);

std::ostream& operator<<(std::ostream& out, const TranslationOptionList& coll)
{
  TranslationOptionList::const_iterator iter;
  for (iter = coll.begin(); iter != coll.end(); ++iter) {
    const TranslationOption &transOpt = **iter;
    out << transOpt << endl;
  }

  return out;
}

size_t
TranslationOptionList::
SelectNBest(size_t const N)
{
  if (N == 0 || N >= m_coll.size()) return 0;
  static TranslationOption::Better cmp;
  NTH_ELEMENT4(m_coll.begin(), m_coll.begin() + N, m_coll.end(), cmp);
  // delete the rest
  for (size_t i = N ; i < m_coll.size() ; ++i) delete m_coll[i];
  size_t ret = m_coll.size() - N;
  m_coll.resize(N);
  return ret;
}

size_t
TranslationOptionList::
PruneByThreshold(float const th)
{
  if (m_coll.size() <= 1) return 0;
  if (th == -std::numeric_limits<float>::infinity()) return 0;

  // first, find the best score
  float bestScore = -std::numeric_limits<float>::infinity();
  BOOST_FOREACH(TranslationOption const* t, m_coll) {
    if (t->GetFutureScore() > bestScore)
      bestScore = t->GetFutureScore();
  }

  size_t old_size = m_coll.size();

  // then, remove items that are worse than best score + threshold
  // why '+' th ??? Does this ever hold?
  for (size_t i=0; i < m_coll.size() ; ++i) {
    if (m_coll[i]->GetFutureScore() < bestScore + th) {
      delete m_coll[i];
      if(i + 1 < m_coll.size())
        std::swap(m_coll[i],m_coll.back());
      m_coll.pop_back();
    }
  }

  m_coll.resize(m_coll.size());
  return old_size - m_coll.size();
}


} // namespace
