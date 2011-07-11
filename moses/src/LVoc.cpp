#include<limits>
#include "LVoc.h"

//rather pointless file because LVoc is template all wee need here is the definitions of consts

const LabelId InvalidLabelId = std::numeric_limits<LabelId>::max();
const LabelId Epsilon        = InvalidLabelId-1;
