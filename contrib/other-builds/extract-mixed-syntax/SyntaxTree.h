#pragma once 

#include <vector>
#include <map>
#include <string>

class SyntaxTree
{
public:
  typedef std::pair<int, int> Range;
  typedef std::vector<std::string> Labels;
  typedef std::map<Range, Labels> Coll;

  typedef Coll::iterator iterator;
  typedef Coll::const_iterator const_iterator;
  //! iterators
  const_iterator begin() const {
	return m_coll.begin();
  }
  const_iterator end() const {
	return m_coll.end();
  }

  void Add(int startPos, int endPos, const std::string &label);
protected:

  Coll m_coll;
};


