//Fabienne Braune
//Alignment info extended to handle one-to-many alignments between one source phrase and a sequence of target phrases

//Commments on implementation :
//Specificity of l-MBOT : we have to handle one-to-many alignments between one source phrase and a sequence of target phrases
//for each non-terminal in a target sequence, we have a source index
//e.g. in [MBOT] |||  [NP][NP] [NP][VP][PP]  [S] ||| [NP][NP]  [NP] || [NP][VP]  [VP] || [NP][PP]  [PP] ||| 0-0 || 1-0 || 1-0
//so we have to uniquely represent 1-0 and 1-0 alignments. We do this by transforming 0-0 1-0 1-0 into 0-0 1-0 2-0
//the source non-terminal NP is aligned to both, a VP and a PP, that is why in the alignments (0-0,1-0,1-0), the source index 1 appears twice
//Also in continguous l-MBOT rules, we have one-to-many alignments this is because we have to plug the discontiguous targets into parent rules
//e.g. in [MBOT] |||  [WHADVP][PWAV] [S][NP][VP][VMFIN]  [SBAR] ||| [WHADVP][PWAV] [S][NP] ­ [S][VP] [S][VMFIN]  [S] ||| 0-0 1-1 1-3 1-4
//so we have to extend the non-term index map to handle multiple source indices.


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

  // Like Alignment Info, provides a map from target-side to source-side non-terminal indices.
  // Because l-MBOT alignments have to handle one-to-many alignments, several components have been modified (see below)

  const NonTermIndexMapPointer GetNonTermIndexMap() const;

  std::vector< const std::pair<size_t,size_t>* > GetSortedAlignments() const;

  //To handle one-to-many alignments, we have to specify all source non-terminals as well as if we are working with a sequence of target sides.
  explicit AlignmentInfoMBOT(const std::set<std::pair<size_t,size_t> > &pairs,
                          std::set<size_t> allSources, bool isSequence)
    : m_collection(pairs), m_nonTermIndexMap( new NonTermIndexMap )
  {
    BuildNonTermIndexMapMBOT(allSources, isSequence);
  }

  ~AlignmentInfoMBOT(){};

 private:

 void BuildNonTermIndexMapMBOT(std::set<size_t> allSources, bool isSequence);
 CollType m_collection;

 //Mbot alignment Info needs a persistant NonTermIndexMap : needs to live after it has been returned
  NonTermIndexMapPointer m_nonTermIndexMap;
};

// Define an arbitrary strict weak ordering between AlignmentInfo objects
// For l-MBOT alignments we also need the nonTermIndexMap to uniquely identify an alignment object
struct AlignmentInfoOrdererMBOT
{
  //do not only take into account the collection but also the non-term index map
  bool operator()(const AlignmentInfoMBOT &a, const AlignmentInfoMBOT &b) const {

	bool isSmaller = 0;
	std::vector<size_t> :: iterator itr_non_term_map_a;
	std::vector<size_t> :: iterator itr_non_term_map_b;
	//if the m_collections are different, then return the difference
	if(a.m_collection != b.m_collection){
		return (a.m_collection < b.m_collection);
	}
	else
	{
		//if the size of the non-term-index-map is different, then return the difference
		if(a.m_nonTermIndexMap->size() != b.m_nonTermIndexMap->size())
		{
			return (a.m_nonTermIndexMap->size() < b.m_nonTermIndexMap->size());
		}
		//otherwise compare all elements in the nonTermIndex map
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

#endif
