#pragma once

#include "moses/ScoreComponentCollection.h"
#include "moses/TargetPhrase.h"

namespace Moses
{
namespace Syntax
{

// A SHyperedge label.
//
struct SLabel {
  // Deltas for individual feature scores.  i.e. this object records the change
  // in each feature score that results from applying the rule associated with
  // this hyperedge.
  ScoreComponentCollection deltas;

  // Total derivation score to be used for comparison in beam search (i.e.
  // including future cost estimates).  This is the sum of the 1-best
  // subderivations' future scores + deltas.
  float futureScore;

  // Target-side of the grammar rule.
  const TargetPhrase *translation;

  // Input weight of this hyperedge (e.g. from weighted input forest).
  float inputWeight;
};

}  // Syntax
}  // Moses
