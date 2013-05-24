// $Id$

#ifndef moses_DummyScoreProducers_h
#define moses_DummyScoreProducers_h

#include "StatelessFeatureFunction.h"

namespace Moses
{

class WordsRange;


/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
public:
	UnknownWordPenaltyProducer(const std::string &line)
  : StatelessFeatureFunction("UnknownWordPenalty",1, line)
  {
	  m_tuneable = false;
  }

};

}

#endif
