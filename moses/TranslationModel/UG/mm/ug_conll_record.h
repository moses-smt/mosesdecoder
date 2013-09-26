#ifndef __ug_conll_record_h
#define __ug_conll_record_h
#include "ug_typedefs.h"
// Base class for dependency tree corpora with POS and Lemma annotations

namespace ugdiss 
{
  using namespace std;

  class 
  Conll_Record 
  {
  public:
    id_type   sform; // surface form
    id_type   lemma; // lemma
    uchar    majpos; // major part of speech
    uchar    minpos; // minor part of speech
    short    parent; // id of parent 
    uchar     dtype; // dependency type
    uchar   info[3]; /* additional information (depends on the part of speech)
                      * a place holder for the time being, to ensure proper 
                      * alignment in memory */
    Conll_Record();
    Conll_Record const* up(int length=1) const;

    Conll_Record& operator=(Conll_Record const& other);

    bool isDescendentOf(Conll_Record const* other) const;

    // virtual bool operator==(Conll_Record const& other) const;
    // virtual bool operator<(Conll_Record const& other) const;
    Conll_Record remap(vector<id_type const*> const& m) const;

#if 0
    /** constructor for conversion from CONLL-stype text format
     *  @parameter SF Vocabulary for surface form
     *  @parameter LM Vocabulary for lemma
     *  @parameter PS Vocabulary for part-of-speech
     *  @parameter DT Vocabulary for dependency type
     */
    Conll_Record(string const& line, 
                 TokenIndex const& SF, TokenIndex const& LM, 
                 TokenIndex const& PS, TokenIndex const& DT);

    /** store the record as-is to disk (for memory-mapped reading later) */
    void store(ostream& out);
#endif
  };

  template<typename T>
  T const* as(Conll_Record const* p)
  {
    return reinterpret_cast<T const*>(p);
  }

  template<typename T>
  T const* up(T const* p,int length=1)
  {
    return as<T>(p->up(length));
  }

  // this is for contigous word sequences extracted from longer sequences
  // adjust parent pointers to 0 (no parent) if they point out of the
  // subsequence
  void 
  fixParse(Conll_Record* start, Conll_Record* stop);

} // end of namespace ugdiss

#endif
