#pragma once

#include <vector>

#include <boost/shared_ptr.hpp>

#include "SHyperedge.h"
#include "SVertex.h"

namespace Moses
{
namespace Syntax
{

typedef std::vector<boost::shared_ptr<SVertex> > SVertexStack;

struct SVertexStackContentOrderer {
public:
  bool operator()(const boost::shared_ptr<SVertex> &x,
                  const boost::shared_ptr<SVertex> &y) {
    return x->best->label.futureScore > y->best->label.futureScore;
  }
};

}  // Syntax
}  // Moses
