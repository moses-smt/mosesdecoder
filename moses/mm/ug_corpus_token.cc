#include "ug_corpus_token.h"
// Simple wrapper around integer IDs for use with the Ctrack and TSA template classes.
// (c) 2007-2009 Ulrich Germann

namespace ugdiss
{
  id_type const&
  SimpleWordId::
  id() const 
  { 
    return theID; 
  }

  int
  SimpleWordId::
  cmp(SimpleWordId const& other) const
  {
    return (theID < other.theID    ? -1
            : theID == other.theID ?  0
            :                         1);
  }

  SimpleWordId::
  SimpleWordId(id_type const& id)
  {
    theID = id;
  }

  bool
  SimpleWordId::
  operator==(SimpleWordId const& other) const
  {
    return theID == other.theID;
  }

  id_type
  SimpleWordId::
  remap(vector<id_type const*> const& m) const
  {
    if (!m[0]) return theID;
    return m[0][theID];
  }

}
