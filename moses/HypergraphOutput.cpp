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

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <util/exception.hh>

#include "ChartHypothesisCollection.h"
#include "ChartManager.h"
#include "HypergraphOutput.h"
#include "Manager.h"

using namespace std;

namespace Moses
{

template class HypergraphOutput<Manager>;
template class HypergraphOutput<ChartManager>;

void
ChartSearchGraphWriterMoses::
WriteHypos(const ChartHypothesisCollection& hypos,
           const map<unsigned, bool> &reachable) const
{

  ChartHypothesisCollection::const_iterator iter;
  for (iter = hypos.begin() ; iter != hypos.end() ; ++iter) {
    ChartHypothesis &mainHypo = **iter;
    if (m_options->output.DontPruneSearchGraph ||
        reachable.find(mainHypo.GetId()) != reachable.end()) {
      (*m_out) << m_lineNumber << " " << mainHypo << endl;
    }

    const ChartArcList *arcList = mainHypo.GetArcList();
    if (arcList) {
      ChartArcList::const_iterator iterArc;
      for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
        const ChartHypothesis &arc = **iterArc;
        if (reachable.find(arc.GetId()) != reachable.end())
          (*m_out) << m_lineNumber << " " << arc << endl;
      }
    }
  }
}

void
ChartSearchGraphWriterHypergraph::
WriteHeader(size_t winners, size_t losers) const
{
  (*m_out) << "# target ||| features ||| source-covered" << endl;
  (*m_out) << winners <<  " " << (winners+losers) << endl;
}

void
ChartSearchGraphWriterHypergraph::
WriteHypos(const ChartHypothesisCollection& hypos,
           const map<unsigned, bool> &reachable) const
{

  ChartHypothesisCollection::const_iterator iter;
  for (iter = hypos.begin() ; iter != hypos.end() ; ++iter) {
    const ChartHypothesis* mainHypo = *iter;
    if (!m_options->output.DontPruneSearchGraph &&
        reachable.find(mainHypo->GetId()) == reachable.end()) {
      //Ignore non reachable nodes
      continue;
    }
    (*m_out) << "# node " << m_nodeId << endl;
    m_hypoIdToNodeId[mainHypo->GetId()] = m_nodeId;
    ++m_nodeId;
    vector<const ChartHypothesis*> edges;
    edges.push_back(mainHypo);
    const ChartArcList *arcList = (*iter)->GetArcList();
    if (arcList) {
      ChartArcList::const_iterator iterArc;
      for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
        const ChartHypothesis* arc = *iterArc;
        if (reachable.find(arc->GetId()) != reachable.end()) {
          edges.push_back(arc);
        }
      }
    }
    (*m_out) << edges.size() << endl;
    for (vector<const ChartHypothesis*>::const_iterator ei = edges.begin();
         ei != edges.end(); ++ei) {
      const ChartHypothesis* hypo = *ei;
      const TargetPhrase& target = hypo->GetCurrTargetPhrase();
      size_t ntIndex = 0;
      for (size_t i = 0; i < target.GetSize(); ++i) {
        const Word& word = target.GetWord(i);
        if (word.IsNonTerminal()) {
          size_t hypoId = hypo->GetPrevHypos()[ntIndex++]->GetId();
          (*m_out) << "[" << m_hypoIdToNodeId[hypoId] << "]";
        } else {
          (*m_out) << word.GetFactor(0)->GetString();
        }
        (*m_out) << " ";
      }
      (*m_out) << " ||| ";
      ScoreComponentCollection scores = hypo->GetScoreBreakdown();
      HypoList::const_iterator hi;
      for (hi = hypo->GetPrevHypos().begin(); hi != hypo->GetPrevHypos().end(); ++hi) {
        scores.MinusEquals((*hi)->GetScoreBreakdown());
      }
      scores.Save(*m_out, false);
      (*m_out) << " ||| ";
      (*m_out) << hypo->GetCurrSourceRange().GetNumWordsCovered();
      (*m_out) << endl;

    }
  }
}


} //namespace Moses

