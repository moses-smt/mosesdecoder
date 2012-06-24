/*******************************************
DAMT Hiero : Features passed to classifier
********************************************/

#ifndef moses_ClassExample_h
#define moses_ClassExample_h

#include <vector>
#include <iterator>
#include <string>
#include "Word.h"
#include "AlignmentInfo.h"

#include <boost/unordered_map.hpp>

namespace Moses
{
    class ClassExample
    {
        protected :

        //strings to be changed into Moses::Word when working with factors
      const std::vector<Word> m_sourceContext;
        const std::vector<std::string> m_sourceSideOfRule;
        const std::vector<std::string> m_targetSideOfRule;
        const AlignmentInfo * m_alignments;


        public :
        ClassExample(
                     const std::vector<Word> &sourceContext,
                     const std::vector<std::string> &sourceSideOfRule,
                     const std::vector<std::string> &targetSideOfRule,
                     const AlignmentInfo * alignments)
                     :m_sourceContext(sourceContext),
                     m_sourceSideOfRule(sourceSideOfRule),
                     m_targetSideOfRule(targetSideOfRule),
                     m_alignments(alignments)
                     {};
       ~ClassExample(){};

       const std::vector<Word> GetSourceContext() const
       { return m_sourceContext;};
       const std::vector<std::string> GetSourceSideOfRule() const
       { return m_sourceSideOfRule;};
       const std::vector<std::string> GetTargetSideOfRule() const
       { return m_targetSideOfRule;};
       const AlignmentInfo * GetAlignments() const
       { return m_alignments;};

    };

    class ClassExampleEqualityPred
    {
        public:
        bool operator()(const ClassExample & ce1,
                  const ClassExample & ce2) const {
        // Compare first non-terminal of each key.  Assumes that for Words
        // representing non-terminals only the first factor is relevant.
        std::vector<Word> :: const_iterator itr_source_context_1;
        std::vector<Word> :: const_iterator itr_source_context_2;
        std::vector<std::string> :: const_iterator itr_source_side_1;
        std::vector<std::string> :: const_iterator itr_source_side_2;
        std::vector<std::string> :: const_iterator itr_target_side_1;
        std::vector<std::string> :: const_iterator itr_target_side_2;

	// Set factors for comparing words
	std::vector<FactorType> srcFactors;
	srcFactors.push_back(0);

        //Compare sizes
        //BEWARE : assumes that the "amount" of context is the same for each example
        if(ce1.GetSourceContext().size() != ce2.GetSourceContext().size())
        {
            return false;
        }

        if(ce1.GetSourceSideOfRule().size() != ce2.GetSourceSideOfRule().size())
        {
            return false;
        }

        if(ce1.GetTargetSideOfRule().size() != ce2.GetTargetSideOfRule().size())
        {
            return false;
        }

        if(ce1.GetAlignments() != ce2.GetAlignments())
        {
            return false;
        }

        //Compare source contexts
        for(
            itr_source_context_1 = ce1.GetSourceContext().begin(),
            itr_source_context_2 = ce2.GetSourceContext().begin();
            itr_source_context_1 != ce1.GetSourceContext().end(),
            itr_source_context_2 != ce2.GetSourceContext().end();
            itr_source_context_1++,
            itr_source_context_2++)
            {
	      if( (*itr_source_context_1).GetString(srcFactors,0).compare((*itr_source_context_2).GetString(srcFactors,0)) != 0 )
                {
                    return false;
                }
            }


            //Compare source sides
            for(
                itr_source_side_1 = ce1.GetSourceSideOfRule().begin(),
                itr_source_side_2 = ce2.GetSourceSideOfRule().begin();
                itr_source_side_1 != ce1.GetSourceSideOfRule().end(),
                itr_source_side_2 != ce2.GetSourceSideOfRule().end();
                itr_source_side_1++,
                itr_source_side_2++)
            {
                if( (*itr_source_side_1).compare(*itr_source_side_2) != 0 )
                {
                    return false;
                }
            }

            //Compare target sides
            for(
                itr_target_side_1 = ce1.GetTargetSideOfRule().begin(),
                itr_target_side_2 = ce2.GetTargetSideOfRule().begin();
                itr_target_side_1 != ce1.GetTargetSideOfRule().end(),
                itr_target_side_2 != ce2.GetTargetSideOfRule().end();
                itr_target_side_1++,
                itr_target_side_2++)
            {
                if( (*itr_target_side_1).compare(*itr_target_side_2) != 0 )
                {
                    return false;
                }
            }
	    
	    //if everything is the same, return true
	    return true;
        }
    };

    class ClassExampleKeyHasher
    {
        public:
            size_t operator()(const ClassExample & ce) const {

            std::vector<Word> :: const_iterator itr_source_context;
            std::vector<std::string> :: const_iterator itr_source_side;
            std::vector<std::string> :: const_iterator itr_target_side;
            std::vector<std::string> :: const_iterator itr_alignment;

            size_t seed = 0;
            std::string s;
            std::vector<FactorType> srcFactors;
	    srcFactors.push_back(0);

	    for(
                itr_source_context = ce.GetSourceContext().begin();
                itr_source_context != ce.GetSourceContext().end();
                itr_source_context++)
            {
	      s = ((*itr_source_context).GetString(srcFactors,0));
                //hash first factor associated to word, can consider more
                boost::hash_combine(seed,s);
            }

            for(
                itr_source_side = ce.GetSourceSideOfRule().begin();
                itr_source_side != ce.GetSourceSideOfRule().end();
                itr_source_side++)
            {
                s  = *itr_source_side;
                //hash first factor associated to word, can consider more
                boost::hash_combine(seed,s);
            }

            for(
                itr_target_side = ce.GetTargetSideOfRule().begin();
                itr_target_side != ce.GetTargetSideOfRule().end();
                itr_target_side++)
            {
                s  = *itr_target_side;
                //hash first factor associated to word, can consider more
                boost::hash_combine(seed,s);
            }

            //combine with alignment
            const AlignmentInfo * a = ce.GetAlignments();
            boost::hash_combine(seed,a);

        return seed;
        }
    };
}
#endif
