//
//  ExtractedRule.cpp
//  extract
//
//  Created by Hieu Hoang on 13/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "ExtractedRule.h"

using namespace std;

namespace MosesTraining
{

void ExtractedRule::OutputNTLengths(std::ostream &out) const
{
  ostringstream outString;
  OutputNTLengths(outString);
  out << outString;
}

void ExtractedRule::OutputNTLengths(std::ostringstream &outString) const
{
  std::map<size_t, std::pair<size_t, size_t> >::const_iterator iter;
  for (iter = m_ntLengths.begin(); iter != m_ntLengths.end(); ++iter)
  {
    size_t sourcePos = iter->first;
    const std::pair<size_t, size_t> &spanLengths = iter->second;
    outString << sourcePos << "=" << spanLengths.first << "," <<spanLengths.second << " "; 
  }
}

std::ostream& operator<<(std::ostream &out, const ExtractedRule &obj)
{
  out << obj.source << " ||| " << obj.target << " ||| " 
      << obj.alignment << " ||| "
      << obj.alignmentInv << " ||| ";
  
  obj.OutputNTLengths(out);

  return out;
}

} // namespace
