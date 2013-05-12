//Fabienne Braune
//Alignment info extended to handle one-to-many alignments between one source phrase and a sequence of target phrases

#include <algorithm>
#include "util/check.hh"
#include "AlignmentInfoMBOT.h"
#include "TypeDef.h"
#include "StaticData.h"

namespace Moses
{

typedef std::vector<size_t> NonTermIndexMap;
typedef boost::shared_ptr<NonTermIndexMap> NonTermIndexMapPointer;

const NonTermIndexMapPointer AlignmentInfoMBOT::GetNonTermIndexMap() const {
    return m_nonTermIndexMap;
  }


void AlignmentInfoMBOT::BuildNonTermIndexMapMBOT(std::set<size_t> allSources, bool isSequence)
{
  //std::cout << "Building non term index map mbot" << std::endl;
  if (m_collection.empty()) {
    //std::cout << "Collection is Empty!" << std::endl;
    return;
  }
  const_iterator p = begin();
  size_t maxIndex = p->second;

  for (++p;  p != end(); ++p) {
    if (p->second > maxIndex) {
      maxIndex = p->second;
    }
  }
  m_nonTermIndexMap->resize(maxIndex+1, NOT_FOUND);
  //Check if this is an alignment between multiple targets or not
  if(!isSequence) //Handle alignment info in non-sequential target phrase (like the usual one)
  {
    size_t i = 0;

      //to check if a given source index has already been seen
      bool PreviousNotSeen = 0;
      bool PreviousSeen = 0;

      //set of source indices, removes duplicates
      std::set<size_t> sources;

      for (p = begin(); p != end(); ++p) {

    	//returns true if the index is not already in the set
        if(sources.insert(p->first).second) // this index has not been seen already
        {

           if(PreviousSeen == 1) //checks if we switch from same source index to new one (e.g. last alignment in 0-1 0-2 1-3)
           {
                i++;
                PreviousSeen = 0;
           }
           m_nonTermIndexMap->at(p->second) = i++; //increment for next alignment
           PreviousNotSeen = 1;
        }
        else  //this index is already in the set
        {
           if(PreviousNotSeen == 1) //checks if we switch from new source index to same one (e.g. second alignment in 0-1 0-2 1-3)
           {
        	   	i--;
                PreviousNotSeen = 0;
           }
           m_nonTermIndexMap->at(p->second) = i;
           PreviousSeen = 1;
        }
      }
    }
    else //Handle alignment info in a sequential target phrase
    {
    	//determine biggest source index
        int maxSourceIndex = 0;

        std::set<size_t>::iterator itr_sources;
        for(itr_sources = allSources.begin(); itr_sources != allSources.end(); itr_sources++)
        {
            if((*itr_sources) > maxSourceIndex)
            {
                maxSourceIndex = *itr_sources;
            }

        }

        //count how many times each source index has been used
        int target = 0;
        int source = 0;
        std::vector<int> countVector (maxSourceIndex + 1,0);
        std::vector<int> :: iterator itr_countVec;
        //std::cout << "Size of Count Vector : " << countVector.size() << std::endl;
        for(itr_sources = allSources.begin(); itr_sources != allSources.end(); itr_sources++)
        {
            //mark each source in count vector
            countVector[*itr_sources] = 1;
        }

        int sum = 0;
        //position in the count vector
        int posCounter = 0;

        //iterate over all alignments
        for (p = begin(); p != end(); ++p) {

        	//look into the count vector until source index is reached
            for(itr_countVec = countVector.begin(); itr_countVec != countVector.end(); itr_countVec++)
            {
            	//source index reached, go to next iteration
                if( posCounter == p->first)
                {
                    target = p->second;
                    //source index becomes count of all source indices at this point
                    source = sum;
                    break;
                }
                //for each source index, sum counts
                sum+=countVector[posCounter];
                posCounter++;
            }
            m_nonTermIndexMap->at(target) = source;
        }
        countVector.clear();
    }
}


  bool compare_target_in_mbot(const std::pair<size_t,size_t> *a, const std::pair<size_t,size_t> *b) {
  if(a->second < b->second)  return true;
  if(a->second == b->second) return (a->first < b->first);
  return false;
}


std::vector< const std::pair<size_t,size_t>* > AlignmentInfoMBOT::GetSortedAlignments() const
{
  std::vector< const std::pair<size_t,size_t>* > ret;

  CollType::const_iterator iter;
  for (iter = m_collection.begin(); iter != m_collection.end(); ++iter)
  {
    const std::pair<size_t,size_t> &alignPair = *iter;
    ret.push_back(&alignPair);
  }

  const StaticData &staticData = StaticData::Instance();
  WordAlignmentSort wordAlignmentSort = staticData.GetWordAlignmentSort();

  switch (wordAlignmentSort)
  {
    case NoSort:
      break;

    case TargetOrder:
      std::sort(ret.begin(), ret.end(), compare_target_in_mbot);
      break;

    default:
      CHECK(false);
  }

  return ret;

}

std::ostream& operator<<(std::ostream &out, const AlignmentInfoMBOT &alignmentInfo)
{
  AlignmentInfo::const_iterator iter;
  for (iter = alignmentInfo.begin(); iter != alignmentInfo.end(); ++iter) {
    out << iter->first << "-" << iter->second << " ";
  }
  out << std::endl;
  //out << "Alignment map : ";
  //std::vector<size_t> :: const_iterator itr_map;
  //for(itr_map = alignmentInfo.GetNonTermIndexMap()->begin(); itr_map != alignmentInfo.GetNonTermIndexMap()->end(); itr_map++)
  //{ out << *itr_map << std::endl; }
  return out;
}

}


