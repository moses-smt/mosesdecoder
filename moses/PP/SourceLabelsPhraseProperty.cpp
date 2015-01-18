#include "moses/PP/SourceLabelsPhraseProperty.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <queue>
#include <assert.h>
#include <limits>

namespace Moses
{

void SourceLabelsPhraseProperty::ProcessValue(const std::string &value)
{
  std::istringstream tokenizer(value);

  if (! (tokenizer >> m_nNTs)) { // first token: number of non-terminals (incl. left-hand side)
    UTIL_THROW2("SourceLabelsPhraseProperty: Not able to read number of non-terminals. Flawed property? " << value);
  }
  assert( m_nNTs > 0 );

  if (! (tokenizer >> m_totalCount)) { // second token: overall rule count
    UTIL_THROW2("SourceLabelsPhraseProperty: Not able to read overall rule count. Flawed property? " << value);
  }
  assert( m_totalCount > 0.0 );



  // read source-labelled rule items

  std::priority_queue<float> ruleLabelledCountsPQ;

  while (tokenizer.peek() != EOF) {
//    try {

    SourceLabelsPhrasePropertyItem item;
    size_t numberOfLHSsGivenRHS = std::numeric_limits<std::size_t>::max();

    if (m_nNTs == 1) {

      item.m_sourceLabelsRHSCount = m_totalCount;

    } else { // rule has right-hand side non-terminals, i.e. it's a hierarchical rule

      for (size_t i=0; i<m_nNTs-1; ++i) { // RHS source non-terminal labels
        size_t sourceLabelRHS;
        if (! (tokenizer >> sourceLabelRHS) ) { // RHS source non-terminal label
          UTIL_THROW2("SourceLabelsPhraseProperty: Not able to read right-hand side label index. Flawed property? " << value);
        }
        item.m_sourceLabelsRHS.push_back(sourceLabelRHS);
      }

      if (! (tokenizer >> item.m_sourceLabelsRHSCount)) {
        UTIL_THROW2("SourceLabelsPhraseProperty: Not able to read right-hand side count. Flawed property? " << value);
      }

      if (! (tokenizer >> numberOfLHSsGivenRHS)) {
        UTIL_THROW2("SourceLabelsPhraseProperty: Not able to read number of left-hand sides. Flawed property? " << value);
      }
    }

    for (size_t i=0; i<numberOfLHSsGivenRHS && tokenizer.peek()!=EOF; ++i) { // LHS source non-terminal labels seen with this RHS
      size_t sourceLabelLHS;
      if (! (tokenizer >> sourceLabelLHS)) { // LHS source non-terminal label
        UTIL_THROW2("SourceLabelsPhraseProperty: Not able to read left-hand side label index. Flawed property? " << value);
      }
      float ruleSourceLabelledCount;
      if (! (tokenizer >> ruleSourceLabelledCount)) {
        UTIL_THROW2("SourceLabelsPhraseProperty: Not able to read count. Flawed property? " << value);
      }
      item.m_sourceLabelsLHSList.push_back( std::make_pair(sourceLabelLHS,ruleSourceLabelledCount) );
      ruleLabelledCountsPQ.push(ruleSourceLabelledCount);
    }

    m_sourceLabelItems.push_back(item);

//    } catch (const std::exception &e) {
//      UTIL_THROW2("SourceLabelsPhraseProperty: Read error. Flawed property?");
//    }
  }

  // keep only top N label vectors
  const size_t N=50;

  if (ruleLabelledCountsPQ.size() > N) {

    float topNRuleLabelledCount = std::numeric_limits<int>::max();
    for (size_t i=0; !ruleLabelledCountsPQ.empty() && i<N; ++i) {
      topNRuleLabelledCount = ruleLabelledCountsPQ.top();
      ruleLabelledCountsPQ.pop();
    }

    size_t nKept=0;
    std::list<SourceLabelsPhrasePropertyItem>::iterator itemIter=m_sourceLabelItems.begin();
    while (itemIter!=m_sourceLabelItems.end()) {
      if (itemIter->m_sourceLabelsRHSCount < topNRuleLabelledCount) {
        itemIter = m_sourceLabelItems.erase(itemIter);
      } else {
        std::list< std::pair<size_t,float> >::iterator itemLHSIter=(itemIter->m_sourceLabelsLHSList).begin();
        while (itemLHSIter!=(itemIter->m_sourceLabelsLHSList).end()) {
          if (itemLHSIter->second < topNRuleLabelledCount) {
            itemLHSIter = (itemIter->m_sourceLabelsLHSList).erase(itemLHSIter);
          } else {
            if (nKept >= N) {
              itemLHSIter = (itemIter->m_sourceLabelsLHSList).erase(itemLHSIter,(itemIter->m_sourceLabelsLHSList).end());
            } else {
              ++nKept;
              ++itemLHSIter;
            }
          }
        }
        if ((itemIter->m_sourceLabelsLHSList).empty()) {
          itemIter = m_sourceLabelItems.erase(itemIter);
        } else {
          ++itemIter;
        }
      }
    }
  }
};

} // namespace Moses

