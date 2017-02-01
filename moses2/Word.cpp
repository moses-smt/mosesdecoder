/*
 * Word.cpp
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */
#include <boost/functional/hash_fwd.hpp>
#include <sstream>
#include <vector>
#include "Word.h"
#include "System.h"
#include "legacy/Util2.h"
#include "util/murmur_hash.hh"

using namespace std;

namespace Moses2
{

Word::Word()
{
  Init<const Factor*>(m_factors, MAX_NUM_FACTORS, NULL);
}

Word::Word(const Word &copy)
{
  memcpy(m_factors, copy.m_factors, sizeof(const Factor *) * MAX_NUM_FACTORS);
}

Word::~Word()
{
  // TODO Auto-generated destructor stub
}

void Word::CreateFromString(FactorCollection &vocab, const System &system,
                            const std::string &str)
{
  vector<string> toks = Tokenize(str, "|");
  for (size_t i = 0; i < toks.size(); ++i) {
    const string &tok = toks[i];
    //cerr << "tok=" << tok << endl;
    const Factor *factor = vocab.AddFactor(tok, system, false);
    m_factors[i] = factor;
  }

  // null the rest
  for (size_t i = toks.size(); i < MAX_NUM_FACTORS; ++i) {
    m_factors[i] = NULL;
  }
}

size_t Word::hash() const
{
  uint64_t seed = 0;
  size_t ret = util::MurmurHashNative(m_factors,
                                      sizeof(Factor*) * MAX_NUM_FACTORS, seed);
  return ret;
}

size_t Word::hash(const std::vector<FactorType> &factors) const
{
  size_t seed = 0;
  for (size_t i = 0; i < factors.size(); ++i) {
    FactorType factorType = factors[i];
    const Factor *factor = m_factors[factorType];
    boost::hash_combine(seed, factor);
  }
  return seed;
}


int Word::Compare(const Word &compare) const
{

  int cmp = memcmp(m_factors, compare.m_factors,
                   sizeof(Factor*) * MAX_NUM_FACTORS);
  return cmp;

  /*
   int ret = m_factors[0]->GetString().compare(compare.m_factors[0]->GetString());
   return ret;
   */
}

bool Word::operator<(const Word &compare) const
{
  int cmp = Compare(compare);
  return (cmp < 0);
}

std::string Word::Debug(const System &system) const
{
  stringstream out;
  bool outputAlready = false;
  for (size_t i = 0; i < MAX_NUM_FACTORS; ++i) {
    const Factor *factor = m_factors[i];
    if (factor) {
      if (outputAlready) {
        out << "|";
      }
      out << *factor;
      outputAlready = true;
    }
  }

  return out.str();
}

void Word::OutputToStream(const System &system, std::ostream &out) const
{
  const std::vector<FactorType> &factorTypes = system.options.output.factor_order;
  out << *m_factors[ factorTypes[0] ];

  for (size_t i = 1; i < factorTypes.size(); ++i) {
    FactorType factorType = factorTypes[i];
    const Factor *factor = m_factors[factorType];

    out << "|" << *factor;
  }
}

std::string Word::GetString(const FactorList &factorTypes) const
{
  assert(factorTypes.size());
  std::stringstream ret;

  ret << m_factors[factorTypes[0]]->GetString();
  for (size_t i = 1; i < factorTypes.size(); ++i) {
    FactorType factorType = factorTypes[i];
    ret << "|" << m_factors[factorType];
  }
  return ret.str();
}

}

