//Fabienne Braune
//Collection of alignment infos specific to l-MBOT
//More explanations in AlignmentInfoMBOT.h

#include "AlignmentInfoCollectionMBOT.h"

using namespace std;

namespace Moses
{

AlignmentInfoCollectionMBOT AlignmentInfoCollectionMBOT::s_instance;


AlignmentInfoCollectionMBOT::AlignmentInfoCollectionMBOT()
{
  std::set<std::pair<size_t,size_t> > alignment;
  std::set<size_t> sources;
  std::vector<std::set<std::pair<size_t,size_t> > > alignmentVector;
  m_emptyAlignmentInfo = Add(alignment,sources,1);
  m_emptyAlignmentInfoMBOT = AddVector(alignmentVector,sources,1);
}

const AlignmentInfoMBOT &AlignmentInfoCollectionMBOT::GetEmptyAlignmentInfo() const
{
  return *m_emptyAlignmentInfo;
}

const vector<const AlignmentInfoMBOT*> &AlignmentInfoCollectionMBOT::GetEmptyAlignmentInfoVector() const
{
  return *m_emptyAlignmentInfoMBOT;
}

const AlignmentInfoMBOT *AlignmentInfoCollectionMBOT::Add(
    const std::set<std::pair<size_t,size_t> > &pairs, std::set<size_t> allSources, bool isSequence)
{
    std::pair<AlignmentInfoMBOTSet::iterator, bool> ret =
    m_collection.insert(AlignmentInfoMBOT(pairs,allSources,isSequence));
    return &(*ret.first);
}

const vector<const AlignmentInfoMBOT*> *AlignmentInfoCollectionMBOT::AddVector(
    std::vector<std::set<std::pair<size_t,size_t> > > &alignmentVector, std::set<size_t> allSources, bool isSequence)
{
	//create vector of AlignmentInfosMBOT before
	std::vector<const AlignmentInfoMBOT*> alignVector;
	std::vector<std::set<std::pair<size_t,size_t> > > :: iterator itr_align_vector;
	for(itr_align_vector = alignmentVector.begin(); itr_align_vector != alignmentVector.end(); itr_align_vector++)
	{
		//std::cerr << "Added alignment info : " << *(Add((*itr_align_vector), allSources, isMBOT)) << std::endl;
		alignVector.push_back(Add((*itr_align_vector), allSources, isSequence));
	}
    std::pair<AlignmentInfoVectorSet::iterator, bool> ret =
    m_vector_collection.insert(alignVector);
    return &(*ret.first);
}
}
