#include <sstream>

#include "ug_deptree.h"
#include "tpt_tokenindex.h"

using namespace std;
namespace ugdiss
{

  bool 
  Conll_Record::
  isDescendentOf(Conll_Record const* other) const
  {
    Conll_Record const* a = this;
    while (a != other && a->parent) 
      a += a->parent;
    return a==other; 
  }

  Conll_Record&
  Conll_Record::
  operator=(Conll_Record const& o)
  {
    sform   = o.sform;
    lemma   = o.lemma;
    majpos  = o.majpos;
    minpos  = o.minpos;
    parent  = o.parent;
    dtype   = o.dtype;
    info[0] = o.info[0];
    info[1] = o.info[1];
    info[2] = o.info[2];
    return *this;
  }

  Conll_Record::
  Conll_Record()
    : sform(0),lemma(0),majpos(0),minpos(0),parent(0),dtype(0)
  {
    info[0]=0;
    info[1]=0;
    info[2]=0;
  }

  Conll_AllFields::
  Conll_AllFields() 
  : Conll_Record::Conll_Record()
  {};

  int
  Conll_AllFields::
  cmp(Conll_Record const& other) const
  {
    if (sform   != other.sform)   return sform   < other.sform   ? -1 : 1;
    if (lemma   != other.lemma)   return lemma   < other.lemma   ? -1 : 1;
    if (majpos  != other.majpos)  return majpos  < other.majpos  ? -1 : 1;
    if (minpos  != other.minpos)  return minpos  < other.minpos  ? -1 : 1;
    if (dtype   != other.dtype)   return dtype   < other.dtype   ? -1 : 1;
    if (info[0] != other.info[0]) return info[0] < other.info[0] ? -1 : 1;
    if (info[1] != other.info[1]) return info[1] < other.info[1] ? -1 : 1;
    if (info[2] != other.info[2]) return info[2] < other.info[2] ? -1 : 1;
    if (parent  != other.parent)  return parent  < other.parent  ? -1 : 1;
    return 0;
  }

  Conll_WildCard::
  Conll_WildCard() 
  : Conll_Record::Conll_Record()
  {};

  int
  Conll_WildCard::
  cmp(Conll_Record const& other) const
  {
    return 0;
  }

#if 1
  bool
  Conll_AllFields::
  operator==(Conll_AllFields const& other) const
  {
    return (sform == other.sform
            && lemma   == other.lemma
            && majpos  == other.majpos
            && minpos  == other.minpos
            && parent  == other.parent
            && dtype   == other.dtype
            && info[0] == other.info[0]
            && info[1] == other.info[1]
            && info[2] == other.info[2]
            );
  }
#endif

#if 0
  Conll_Record::
  Conll_Record(string const& line, 
               TokenIndex const& SF, TokenIndex const& LM, 
               TokenIndex const& PS, TokenIndex const& DT)
  {

    string        surf,lem,pos1,pos2,dummy,drel;
    short         id,gov;
    istringstream buf(line);

    buf >> id >> surf >> lem >> pos1 >> pos2 >> dummy >> gov >> drel;

    sform  = SF[surf];
    lemma  = LM[lem];
    if (PS[pos1] > 255 || PS[pos2] > 255 || DT[drel] > 255)
      {
        cerr << "error at this line:\n" << line << endl;
        exit(1);
      }
    majpos = rangeCheck(PS[pos1],256);
    minpos = rangeCheck(PS[pos2],256);
    dtype  = rangeCheck(DT[drel],256);
    parent = gov ? gov-id : 0;
    info[0]=info[1]=info[2]=0;
  }
  void
  Conll_Record::
  store(ostream& out)
  {
    out.write(reinterpret_cast<char const*>(this),sizeof(*this));
  }
#endif

#if 1
  Conll_Record const*
  Conll_Record::up(int length) const
  {
    Conll_Record const* ret = this;
    while (length-- > 0)
      if (!ret->parent) return NULL;
      else ret += ret->parent;
    return ret;
  }
#endif

  Conll_Sform::
  Conll_Sform() 
  : Conll_Record::Conll_Record() 
  {};

  Conll_MinPos::
  Conll_MinPos() 
  : Conll_Record::Conll_Record() 
  {};
 
  Conll_MinPos_Lemma::
  Conll_MinPos_Lemma() 
  : Conll_Record::Conll_Record() 
  {};

  Conll_Lemma::
  Conll_Lemma()
  : Conll_Record::Conll_Record() 
  {};

  Conll_Lemma::
  Conll_Lemma(id_type _id)
  : Conll_Record::Conll_Record() 
  {
    this->lemma = _id;
  };

  Conll_MinPos::
  Conll_MinPos(id_type _id)
  : Conll_Record::Conll_Record() 
  {
    this->minpos = _id;
  };

  id_type
  Conll_MinPos::
  id() const
  {
    return this->minpos;
  }

  Conll_MajPos::
  Conll_MajPos(id_type _id)
  : Conll_Record::Conll_Record() 
  {
    this->majpos = _id;
  };

  id_type
  Conll_MajPos::
  id() const
  {
    return this->majpos;
  }

  id_type
  Conll_MinPos_Lemma::
  id() const
  {
    return this->minpos;
  }

  int
  Conll_MajPos::
  cmp(Conll_Record const& other) const
  {
    return this->majpos < other.majpos ? -1 : this->majpos > other.majpos ? 1 : 0;
  }

  int
  Conll_MinPos::
  cmp(Conll_Record const& other) const
  {
    return this->minpos < other.minpos ? -1 : this->minpos > other.minpos ? 1 : 0;
  }

  int
  Conll_MinPos_Lemma::
  cmp(Conll_Record const& other) const
  {
    if (this->minpos != 0 && other.minpos != 0 && this->minpos != other.minpos) 
      return this->minpos < other.minpos ? -1 : 1;
    if (this->lemma != 0 && other.lemma != 0 && this->lemma != other.lemma)
      return this->lemma < other.lemma ? -1 : 1;
    return 0;
  }

  id_type 
  Conll_Lemma::
  id() const 
  { 
    return this->lemma; 
  }

  int 
  Conll_Lemma::
  cmp(Conll_Record const& other) const
  {
#if 0
    for (Conll_Record const* x = this; x; x = x->parent ? x+x->parent : NULL)
      cout << (x!=this?".":"") << x->lemma;
    cout << " <=> ";
    for (Conll_Record const* x = &other; x; x = x->parent ? x+x->parent : NULL)
      cout << (x!=&other?".":"") << x->lemma;
    cout << (this->lemma < other.lemma ? -1 : this->lemma > other.lemma ? 1 : 0);
    cout << endl;
#endif
    return this->lemma < other.lemma ? -1 : this->lemma > other.lemma ? 1 : 0;
  }

  Conll_Sform::
  Conll_Sform(id_type _id)
  : Conll_Record::Conll_Record() 
  {
    this->sform = _id;
  };

  id_type 
  Conll_Sform
  ::id() const 
  { 
    return this->sform; 
  }

  int
  Conll_Sform::
  cmp(Conll_Record const& other) const
  {
    return this->sform < other.sform ? -1 : this->sform > other.sform ? 1 : 0;
  }

#if 0
  dpSnt::
  dpSnt(Conll_Record const* first, Conll_Record const* last)
  {
    w.reserve(last-first);
    for (Conll_Record const* x = first; x < last; ++x)
      w.push_back(DTNode(x));
    for (size_t i = 0; i < w.size(); i++)
      {
        short p = w[i].rec->parent;
        if (p != 0)
          {
            if (p > 0) assert(i+p < w.size()); 
            else       assert(i >= size_t(-p));
            w[i].parent  = &(w[i+p]);
            w[i].parent->children.push_back(&(w[i]));
          }
      }
  }
#endif

  /** @return true if the linear sequence of /Conll_Record/s is coherent, 
   *  i.e., a proper connected tree structure */
  bool
  isCoherent(Conll_Record const* const start, Conll_Record const* const stop)
  {
    int outOfRange=0;
    for (Conll_Record const* x = start; outOfRange <= 1 && x < stop; ++x)
      {
        Conll_Record const* n = x->up();
        if (!n || n < start || n >= stop) 
          outOfRange++;
      }
    return outOfRange<=1;
  }
  
  // this is for contigous word sequences extracted from longer sequences
  // adjust parent pointers to 0 (no parent) if they point out of the
  // subsequence
  void 
  fixParse(Conll_Record* start, Conll_Record* stop)
  {
    int len = stop-start;
    int i = 0;
    for (Conll_Record* x = start; x < stop; ++x,++i)
      {
        int p = i+x->parent;
        if (p < 0 || p >= len) x->parent = 0;
      }
  }
}
