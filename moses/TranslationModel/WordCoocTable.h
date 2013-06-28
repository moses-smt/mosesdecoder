#ifndef moses_WordCoocTable_h
#define moses_WordCoocTable_h

#include "moses/TranslationModel/DynSAInclude/vocab.h"
#include "moses/TranslationModel/DynSAInclude/types.h"
#include "moses/TranslationModel/DynSAInclude/utils.h"
#include "moses/InputFileStream.h"
#include "moses/FactorTypeSet.h"
#include "moses/TargetPhrase.h"
#include <boost/dynamic_bitset.hpp>
#include <map>

namespace Moses
{

using namespace std;

#ifndef bitvector
typedef boost::dynamic_bitset<uint64_t> bitvector;
#endif


/**
 *  Stores word cooccurrence counts
 *  @todo ask Uli Germann
 */
class WordCoocTable
{
  typedef map<wordID_t,uint32_t> my_map_t;
  vector<my_map_t> m_cooc;
  vector<uint32_t> m_marg1;
  vector<uint32_t> m_marg2;
public:
  WordCoocTable();
  WordCoocTable(wordID_t const VocabSize1, wordID_t const VocabSize2);
  uint32_t GetJoint(size_t const a, size_t const b) const;
  uint32_t GetMarg1(size_t const x) const;
  uint32_t GetMarg2(size_t const x) const;
  float pfwd(size_t const a, size_t const b) const;
  float pbwd(size_t const a, size_t const b) const;
  void
  Count(size_t const a, size_t const b);

  template<typename idvec, typename alnvec>
  void
  Count(idvec const& s1, idvec const& s2, alnvec const& aln,
        wordID_t const NULL1, wordID_t const NULL2);

};

template<typename idvec, typename alnvec>
void
WordCoocTable::
Count(idvec const& s1, idvec const& s2, alnvec const& aln,
      wordID_t const NULL1, wordID_t const NULL2)
{
  boost::dynamic_bitset<uint64_t> check1(s1.size()), check2(s2.size());
  check1.set();
  check2.set();
  for (size_t i = 0; i < aln.size(); i += 2) {
    Count(s1[aln[i]], s2[aln[i+1]]);
    check1.reset(aln[i]);
    check2.reset(aln[i+1]);
  }
  for (size_t i = check1.find_first(); i < check1.size(); i = check1.find_next(i))
    Count(s1[i], NULL2);
  for (size_t i = check2.find_first(); i < check2.size(); i = check2.find_next(i))
    Count(NULL1, s2[i]);
}

}
#endif
