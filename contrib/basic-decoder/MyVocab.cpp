
#include "MyVocab.h"
#include "Util.h"
#include "check.h"

using namespace std;

namespace FastMoses
{

MyVocab MyVocab::s_instance;
VOCABID MyVocab::s_currId = 0;

MyVocab::MyVocab()
{
  // TODO Auto-generated constructor stub
}

MyVocab::~MyVocab()
{
  cerr << "delete Vocab" << endl;
}

VOCABID MyVocab::GetOrCreateId(const std::string &str)
{
  Coll::left_map::const_iterator iter;
  iter = m_coll.left.find(str);
  if (iter != m_coll.left.end()) {
    return iter->second;
  } else {
    ++s_currId;
    m_coll.insert(Coll::value_type(str, s_currId));
    return s_currId;
  }
}

const std::string &MyVocab::GetString(VOCABID id) const
{
  Coll::right_map::const_iterator iter;
  iter = m_coll.right.find(id);
  assert(iter != m_coll.right.end());

  const string &str = iter->second;
  return str;
}

}
