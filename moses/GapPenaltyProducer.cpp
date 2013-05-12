// $Id: GapPenaltyProducer.cpp,v 1.1.1.1 2013/01/06 16:54:18 braunefe Exp $

#include "util/check.hh"
#include "FFState.h"
#include "StaticData.h"
#include "GapPenaltyProducer.h"
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

namespace Moses
{

//Fabienne Braune : Just in case Gap penalty is evaluated in the wrong place
void GapPenaltyProducer::Evaluate(const TargetPhrase tp, ScoreComponentCollection* out) const
{
	 std::cout << "Evaluate should not be called here !" << std::endl;
}

size_t GapPenaltyProducer::GetNumScoreComponents() const
{
  return 1;
}

std::string GapPenaltyProducer::GetScoreProducerDescription(unsigned) const
{
  return "GPP";
}

std::string GapPenaltyProducer::GetScoreProducerWeightShortName(unsigned) const
{
  return "gpp";
}

size_t GapPenaltyProducer::GetNumInputScores() const
{
  return 0;
}

} // namespace Moses
