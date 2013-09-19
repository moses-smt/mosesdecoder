/***********************************************************************
 Moses - statistical machine translation system
 Copyright (C) 2006-2011 University of Edinburgh

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_AlignmentInfoMBOT_h
#define moses_AlignmentInfoMBOT_h

#include <ostream>
#include <set>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace Moses
{

class AlignmentInfoCollectionMBOT;

// Collection of non-terminal alignment pairs, ordered by source index.
// Because we use discontiguous target phrases, we have to handle multiple alignments to source (1-to-many)
class AlignmentInfoMBOT
{
  typedef std::set<std::pair<size_t,size_t> > CollType;

  friend std::ostream& operator<<(std::ostream &, const AlignmentInfoMBOT &);
  friend struct AlignmentInfoOrdererMBOT;
  friend class AlignmentInfoCollectionMBOT;

 public:
  typedef std::vector<size_t> NonTermIndexMap;
  typedef CollType::const_iterator const_iterator;
  typedef boost::shared_ptr<NonTermIndexMap> NonTermIndexMapPointer;

  const_iterator begin() const { return m_collection.begin(); }
  const_iterator end() const { return m_collection.end(); }


  const NonTermIndexMapPointer GetNonTermIndexMap() const;

  std::vector< const std::pair<size_t,size_t>* > GetSortedAlignments() const;

  explicit AlignmentInfoMBOT(const std::set<std::pair<size_t,size_t> > &pairs,
                          std::set<size_t> allSources, bool isMBOT)
    : m_collection(pairs), m_nonTermIndexMap( new NonTermIndexMap )
  {
    BuildNonTermIndexMapMBOT(allSources, isMBOT);
  }

  ~AlignmentInfoMBOT(){};

 private:

 void BuildNonTermIndexMapMBOT(std::set<size_t> allSources, bool isMBOT);

  CollType m_collection;

  NonTermIndexMapPointer m_nonTermIndexMap;
};

// Define an arbitrary strict weak ordering between AlignmentInfo objects
// for use by AlignmentInfoCollection.
struct AlignmentInfoOrdererMBOT
{

  //do not only take into account the collection but also the non-term index map
  bool operator()(const AlignmentInfoMBOT &a, const AlignmentInfoMBOT &b) const {

	bool isSmaller = 0;
	std::vector<size_t> :: iterator itr_non_term_map_a;
	std::vector<size_t> :: iterator itr_non_term_map_b;
	//if the m_collections are different, then return the difference
	if(a.m_collection != b.m_collection){
		/*if(isToDebug == 1)
		{
			std::cerr << "Collection is not equal " << std::endl;
			std::cerr << "Returns : " << (a.m_collection < b.m_collection) << std::endl;
		}*/
		return (a.m_collection < b.m_collection);
	}
	else
	{
		//if the size of the non-term-index-map is different, then return the difference
		if(a.m_nonTermIndexMap->size() != b.m_nonTermIndexMap->size())
		{
			return (a.m_nonTermIndexMap->size() < b.m_nonTermIndexMap->size());
		}
		//otherwise compare all elements in the vector
		else
		{
			for(	itr_non_term_map_a = a.m_nonTermIndexMap->begin(),
					itr_non_term_map_b = b.m_nonTermIndexMap->begin();
					itr_non_term_map_a != a.m_nonTermIndexMap->end(),
				    itr_non_term_map_b != b.m_nonTermIndexMap->end();
					itr_non_term_map_a++,itr_non_term_map_b++)
			{

				isSmaller = (*itr_non_term_map_a < *itr_non_term_map_b);
				if(isSmaller == 1)
				{
					return (isSmaller);
				}
			}
			return (isSmaller);
		}
	}
  }
};

}

#endif moses_AlignmentInfoMBOT_h
