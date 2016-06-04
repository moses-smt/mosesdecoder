
#include <boost/algorithm/string.hpp>

#include "M2.h"

namespace MosesTuning
{

namespace M2
{

bool Annot::lowercase = true;

std::string Annot::transform(const std::string& e)
{
  std::string temp = e;
  if(lowercase) {
    boost::erase_all(temp, " ");
    return ToLower(temp);
  } else
    return e;
}

const std::string ToLower(const std::string& str)
{
  std::string lc(str);
  std::transform(lc.begin(), lc.end(), lc.begin(), (int(*)(int))std::tolower);
  return lc;
}


Edit operator+(Edit& e1, Edit& e2)
{
  std::string edit;
  if(e1.edit.size() > 0 && e2.edit.size() > 0)
    edit = e1.edit + " " + e2.edit;
  else if(e1.edit.size() > 0)
    edit = e1.edit;
  else if(e2.edit.size() > 0)
    edit = e2.edit;

  return Edit(e1.cost + e2.cost, e1.changed + e2.changed, e1.unchanged + e2.unchanged, edit);
}


Edge operator+(Edge e1, Edge e2)
{
  return Edge(e1.v, e2.u, e1.edit + e2.edit);
}

std::ostream& operator<<(std::ostream& o, Sentence s)
{
  for(Sentence::iterator it = s.begin(); it != s.end(); it++)
    o << *it << " ";
  return o;
}


}

}