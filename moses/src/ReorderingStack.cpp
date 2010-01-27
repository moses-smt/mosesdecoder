/*
 * ReorderingStack.cpp
 ** Author: Ankit K. Srivastava
 ** Date: Jan 26, 2010
*/

#include "ReorderingStack.h"
#include <vector>

namespace Moses
{
   int ReorderingStack::Compare(const ReorderingStack& o)  const {
      const ReorderingStack& other = static_cast<const ReorderingStack&>(o);
      if (other.m_stack > m_stack) return 1;
      else if (other.m_stack < m_stack) return -1;
      return 0;
   }

   // Method to push (shift element into the stack and reduce if reqd)
   int ReorderingStack::Push(WordsRange input_span)
   {
       int distance;  // value to return: the initial distance between this and previous span

       // stack is empty
       if(m_stack.empty()) {
          m_stack.push_back(input_span);
          return 0;
       }

       // stack is non-empty
       WordsRange prev_span = m_stack.back(); //access last element added

       if(input_span.GetStartPos() > prev_span.GetStartPos()) distance = input_span.GetStartPos() - prev_span.GetEndPos();
       else distance = input_span.GetEndPos() - prev_span.GetStartPos();

       if(distance == 1){ //monotone
           m_stack.pop_back();
           WordsRange new_span(prev_span.GetStartPos(), input_span.GetEndPos());
           Merge(new_span);

       }

       if(distance == -1){ //swap
           m_stack.pop_back();
           WordsRange new_span(input_span.GetStartPos(), prev_span.GetEndPos());
           Merge(new_span);

       }

       // discontinuous
       m_stack.push_back(input_span);


       return distance;

   }

   // Method to merge, if possible the spans
   void ReorderingStack::Merge(WordsRange x){
       int cont_loop = 1;

       while (cont_loop && m_stack.size() > 0) {

          WordsRange p = m_stack.back();

          if(x.GetStartPos() - p.GetEndPos() == 1) { //mono&merge
             m_stack.pop_back();
             WordsRange t(p.GetStartPos(), x.GetEndPos());
             x = t;
          }

          else {
             if(p.GetStartPos() - x.GetEndPos() == 1) { //swap&merge
                m_stack.pop_back();
                WordsRange t(x.GetStartPos(), p.GetEndPos());
                x = t;
             }
             else { // discontinuous, no more merging
                cont_loop=0;
             }
          }
       } // finished merging, exit

       // add to stack
       m_stack.push_back(x);
   }

}

