#include "ug_conll_record.h"
namespace ugdiss
{
  Conll_Record
  Conll_Record::
  remap(vector<id_type const*> const& m) const
  {
    Conll_Record ret;
    ret.sform   = m.size() > 0 && m[0] ? m[0][this->sform]   : this->sform;
    ret.lemma   = m.size() > 1 && m[1] ? m[1][this->lemma]   : this->lemma;
    ret.majpos  = m.size() > 2 && m[2] ? m[2][this->majpos]  : this->majpos;
    ret.minpos  = m.size() > 2 && m[2] ? m[2][this->minpos]  : this->minpos;
    ret.dtype   = m.size() > 3 && m[3] ? m[3][this->dtype]   : this->dtype;
    ret.info[0] = m.size() > 4 && m[4] ? m[4][this->info[0]] : this->info[0];
    ret.info[1] = m.size() > 5 && m[5] ? m[5][this->info[1]] : this->info[1];
    ret.info[2] = m.size() > 6 && m[6] ? m[6][this->info[2]] : this->info[2];
    ret.parent = this->parent;
    return ret;
  }
}
