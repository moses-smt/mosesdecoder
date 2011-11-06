//
//  PhraseDictionaryALSuffixArray.h
//  moses
//
//  Created by Hieu Hoang on 06/11/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef moses_PhraseDictionaryALSuffixArray_h
#define moses_PhraseDictionaryALSuffixArray_h

#include "PhraseDictionaryHiero.h"

namespace Moses {
  
class PhraseDictionaryALSuffixArray : public PhraseDictionaryHiero
{
public:
  PhraseDictionaryHiero(size_t numScoreComponent, PhraseDictionaryFeature* feature)
  : PhraseDictionaryHiero(numScoreComponent,feature) {}
}

}

#endif
