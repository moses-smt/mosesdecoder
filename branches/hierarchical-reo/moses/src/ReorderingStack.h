/*
 * ReorderingStack.h
 ** Author: Ankit K. Srivastava
 ** Date: Jan 26, 2010 
 */

#pragma once

//#include <string>
#include <vector>
//#include "Factor.h"
//#include "Phrase.h"
//#include "TypeDef.h"
//#include "Util.h"
#include "WordsRange.h"
//#include "ScoreProducer.h"
//#include "FeatureFunction.h"

namespace Moses
{

 class ReorderingStack
 {
   private:
	
            std::vector<WordsRange> m_stack;

   public:
	
		int Compare(const ReorderingStack& o) const;

		
		int Push(WordsRange input_span);
                void Merge(WordsRange input_span);
 };	


}
