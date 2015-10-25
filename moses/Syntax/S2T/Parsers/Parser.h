#pragma once

namespace Moses
{
namespace Syntax
{
namespace S2T
{

class PChart;

// Base class for parsers.
template<typename Callback>
class Parser
{
public:
  typedef Callback CallbackType;

  Parser(PChart &chart) : m_chart(chart) {}

  virtual ~Parser() {}

  virtual void EnumerateHyperedges(const Range &, Callback &) = 0;
protected:
  PChart &m_chart;
};

}  // S2T
}  // Syntax
}  // Moses
