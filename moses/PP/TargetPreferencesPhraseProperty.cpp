#include "moses/PP/TargetPreferencesPhraseProperty.h"
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

void TargetPreferencesPhraseProperty::ProcessValue(const std::string &value)
{
  std::istringstream tokenizer(value);

  if (! (tokenizer >> m_nNTs)) { // first token: number of non-terminals (incl. left-hand side)
    UTIL_THROW2("TargetPreferencesPhraseProperty: Not able to read number of non-terminals. Flawed property?");
  }
  assert( m_nNTs > 0 );

  if (! (tokenizer >> m_totalCount)) { // second token: overall rule count
    UTIL_THROW2("TargetPreferencesPhraseProperty: Not able to read overall rule count. Flawed property?");
  }
  assert( m_totalCount > 0.0 );


  // read labelled rule items

  std::priority_queue<float> ruleLabelledCountsPQ;

  while (tokenizer.peek() != EOF) {
    try {

      TargetPreferencesPhrasePropertyItem item;
      size_t numberOfLHSsGivenRHS = std::numeric_limits<std::size_t>::max();

      if (m_nNTs == 1) {

        item.m_labelsRHSCount = m_totalCount;

      } else { // rule has right-hand side non-terminals, i.e. it's a hierarchical rule

        for (size_t i=0; i<m_nNTs-1; ++i) { // RHS non-terminal labels
          size_t labelRHS;
          if (! (tokenizer >> labelRHS) ) { // RHS non-terminal label
            UTIL_THROW2("TargetPreferencesPhraseProperty: Not able to read right-hand side label index. Flawed property?");
          }
          item.m_labelsRHS.push_back(labelRHS);
        }

        if (! (tokenizer >> item.m_labelsRHSCount)) {
          UTIL_THROW2("TargetPreferencesPhraseProperty: Not able to read right-hand side count. Flawed property?");
        }

        if (! (tokenizer >> numberOfLHSsGivenRHS)) {
          UTIL_THROW2("TargetPreferencesPhraseProperty: Not able to read number of left-hand sides. Flawed property?");
        }
      }

      for (size_t i=0; i<numberOfLHSsGivenRHS && tokenizer.peek()!=EOF; ++i) { // LHS non-terminal labels seen with this RHS
        size_t labelLHS;
        if (! (tokenizer >> labelLHS)) { // LHS non-terminal label
          UTIL_THROW2("TargetPreferencesPhraseProperty: Not able to read left-hand side label index. Flawed property?");
        }
        float ruleLabelledCount;
        if (! (tokenizer >> ruleLabelledCount)) {
          UTIL_THROW2("TargetPreferencesPhraseProperty: Not able to read count. Flawed property?");
        }
        item.m_labelsLHSList.push_back( std::make_pair(labelLHS,ruleLabelledCount) );
        ruleLabelledCountsPQ.push(ruleLabelledCount);
      }

      m_labelItems.push_back(item);

    } catch (const std::exception &e) {
      UTIL_THROW2("TargetPreferencesPhraseProperty: Read error. Flawed property?");
    }
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
    std::list<TargetPreferencesPhrasePropertyItem>::iterator itemIter=m_labelItems.begin();
    while (itemIter!=m_labelItems.end()) {
      if (itemIter->m_labelsRHSCount < topNRuleLabelledCount) {
        itemIter = m_labelItems.erase(itemIter);
      } else {
        std::list< std::pair<size_t,float> >::iterator itemLHSIter=(itemIter->m_labelsLHSList).begin();
        while (itemLHSIter!=(itemIter->m_labelsLHSList).end()) {
          if (itemLHSIter->second < topNRuleLabelledCount) {
            itemLHSIter = (itemIter->m_labelsLHSList).erase(itemLHSIter);
          } else {
            if (nKept >= N) {
              itemLHSIter = (itemIter->m_labelsLHSList).erase(itemLHSIter,(itemIter->m_labelsLHSList).end());
            } else {
              ++nKept;
              ++itemLHSIter;
            }
          }
        }
        if ((itemIter->m_labelsLHSList).empty()) {
          itemIter = m_labelItems.erase(itemIter);
        } else {
          ++itemIter;
        }
      }
    }
  }
};

} // namespace Moses

