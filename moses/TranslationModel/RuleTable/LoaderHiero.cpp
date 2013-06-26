//
//  RuleTableLoaderHiero.cpp
//  moses
//
//  Created by Hieu Hoang on 04/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "LoaderHiero.h"

using namespace std;

namespace Moses
{

bool RuleTableLoaderHiero::Load(const std::vector<FactorType> &input,
                                const std::vector<FactorType> &output,
                                const std::string &inFile,
                                size_t tableLimit,
                                RuleTableTrie &ruleTable)
{
  bool ret = RuleTableLoaderStandard::Load(HieroFormat
             ,input, output
             ,inFile
             ,tableLimit
             ,ruleTable);
  return ret;
}

}

