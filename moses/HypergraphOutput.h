// $Id$
// vim:tabstop=2

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2014- University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_Hypergraph_Output_h
#define moses_Hypergraph_Output_h

#include <ostream>
#include "moses/parameters/AllOptions.h"

/**
* Manage the output of hypergraphs.
**/

namespace Moses
{

class ChartHypothesisCollection;

template<class M>
class HypergraphOutput
{

public:
  /** Initialise output directory and create weights file */
  HypergraphOutput(size_t precision);

  /** Write this hypergraph to file */
  void Write(const M& manager) const;

private:
  size_t m_precision;
  std::string m_hypergraphDir;
  std::string m_compression;
  bool m_appendSuffix;
};


/**
 * ABC for different types of search graph output for chart Moses.
**/
class ChartSearchGraphWriter
{
protected:
  AllOptions::ptr m_options;
  ChartSearchGraphWriter(AllOptions::ptr const& opts) : m_options(opts) { }
public:
  virtual void WriteHeader(size_t winners, size_t losers) const = 0;
  virtual void WriteHypos(const ChartHypothesisCollection& hypos,
                          const std::map<unsigned, bool> &reachable) const = 0;

};

/** "Moses" format (osg style) */
class ChartSearchGraphWriterMoses : public virtual ChartSearchGraphWriter
{
public:
  ChartSearchGraphWriterMoses(AllOptions::ptr const& opts,
                              std::ostream* out, size_t lineNumber)
    : ChartSearchGraphWriter(opts), m_out(out), m_lineNumber(lineNumber) {}
  virtual void WriteHeader(size_t, size_t) const {
    /* do nothing */
  }
  virtual void WriteHypos(const ChartHypothesisCollection& hypos,
                          const std::map<unsigned, bool> &reachable) const;

private:
  std::ostream* m_out;
  size_t m_lineNumber;
};

/** Modified version of Kenneth's lazy hypergraph format */
class ChartSearchGraphWriterHypergraph : public virtual ChartSearchGraphWriter
{
public:
  ChartSearchGraphWriterHypergraph(AllOptions::ptr const& opts, std::ostream* out)
    : ChartSearchGraphWriter(opts), m_out(out), m_nodeId(0) { }
  virtual void WriteHeader(size_t winners, size_t losers) const;
  virtual void WriteHypos(const ChartHypothesisCollection& hypos,
                          const std::map<unsigned, bool> &reachable) const;

private:
  std::ostream* m_out;
  mutable size_t m_nodeId;
  mutable std::map<size_t,size_t> m_hypoIdToNodeId;
};

}
#endif
