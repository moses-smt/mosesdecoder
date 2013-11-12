
#pragma once

#include <boost/bimap.hpp>
#include "TypeDef.h"

namespace FastMoses
{

class MyVocab
{
public:
  static MyVocab &Instance() {
    return s_instance;
  }

  MyVocab();
  virtual ~MyVocab();

  VOCABID GetOrCreateId(const std::string &str);
  const std::string &GetString(VOCABID id) const;
protected:
  static MyVocab s_instance;
  static VOCABID s_currId;

  typedef boost::bimap<std::string, VOCABID > Coll;
  Coll m_coll;

};

}
