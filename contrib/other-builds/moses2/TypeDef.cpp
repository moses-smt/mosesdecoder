#include "TypeDef.h"
#include "util/exception.hh"
#include <typeinfo>


namespace Moses2
{

/*  handle extraction operators for the above enums (presuming c++11 not available) */
template <typename T>
inline int read_int_or_fail(std::istream &in) {
  int tmp;
  in >> tmp;
  if(in.fail()) {
    UTIL_THROW(util::Exception, "Unrecognized (non-integer) value for " << typeid(T).name());
  }
  return tmp;
}

// regular, contiguous enums can just be range checked using the min/max enum values
template <typename T>
inline std::istream& read_regular_enum(std::istream &in, T &value, int first, int last) {
  int tmp = read_int_or_fail<T>(in);

  if(tmp < first || tmp > last) {
    UTIL_THROW(util::Exception, "Unrecognized value for " << typeid(T).name());
  }
  else {
    value = T(tmp);
  }
  return in;
}

std::istream& operator>>(std::istream &in, XmlInputType &value) {
  return read_regular_enum(in, value, XmlPassThrough, XmlConstraint);
}
std::istream& operator>>(std::istream &in, WordAlignmentSort &value) {
  return read_regular_enum(in, value, NoSort, TargetOrder);
}
std::istream& operator>>(std::istream &in, S2TParsingAlgorithm &value) {
  return read_regular_enum(in, value, RecursiveCYKPlus, Scope3);
}
std::istream& operator>>(std::istream &in, SourceLabelOverlap &value) {
  return read_regular_enum(in, value, SourceLabelOverlapAdd, SourceLabelOverlapDiscard);
}

// non-contiguos enum values
std::istream& operator>>(std::istream &in, SearchAlgorithm &value) {
  int tmp = read_int_or_fail<SearchAlgorithm>(in);
  
  switch(tmp) {
  case Normal:
  case CubePruning:
  //case CubeGrowing:
  case CYKPlus:
  //case NormalBatch :
  case ChartIncremental:
  case SyntaxS2T:
  case SyntaxT2S:
  case SyntaxT2S_SCFG:
  case SyntaxF2S:
  case CubePruningPerMiniStack:
  case CubePruningPerBitmap:
  case CubePruningCardinalStack:
  case CubePruningBitmapStack:
  case CubePruningMiniStack:
  case DefaultSearchAlgorithm:
	  value = SearchAlgorithm(tmp);
	  break;
  default:
	  UTIL_THROW(util::Exception, "Unrecognized value for SearchAlgoritm");
  };
  return in;
}
std::istream& operator>>(std::istream &in, InputTypeEnum &value) {
  int tmp = read_int_or_fail<InputTypeEnum>(in);

  switch(tmp) {
  case SentenceInput:
  case ConfusionNetworkInput:
  case WordLatticeInput:
  case TreeInputType:
  //case WordLatticeInput2:
  case TabbedSentenceInput:
  case ForestInputType:
    value = InputTypeEnum(tmp);
  default:
    UTIL_THROW(util::Exception, "Unrecognized value for Input Type");
  };
  return in;
}

}
