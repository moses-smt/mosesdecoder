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

template<class M>
HypergraphOutput<M>::HypergraphOutput(size_t precision) :
  m_precision(precision)
{
  const StaticData& staticData = StaticData::Instance();
  vector<string> hypergraphParameters;
  const PARAM_VEC *params = staticData.GetParameter().GetParam("output-search-graph-hypergraph");
  if (params) {
    hypergraphParameters = *params;
  }

  if (hypergraphParameters.size() > 0 && hypergraphParameters[0] == "true") {
    m_appendSuffix = true;
  } else {
    m_appendSuffix = false;
  }

  string compression;
  if (hypergraphParameters.size() > 1) {
    m_compression = hypergraphParameters[1];
  } else {
    m_compression = "txt";
  }
  UTIL_THROW_IF(m_compression != "txt" && m_compression != "gz" && m_compression != "bz2",
                util::Exception, "Unknown compression type: " << m_compression);

  if ( hypergraphParameters.size() > 2 ) {
    m_hypergraphDir = hypergraphParameters[2];
  } else {
    string nbestFile = staticData.GetNBestFilePath();
    if ( ! nbestFile.empty() && nbestFile!="-" && !boost::starts_with(nbestFile,"/dev/stdout") ) {
      boost::filesystem::path nbestPath(nbestFile);

      // In the Boost filesystem API version 2,
      //   which was the default prior to Boost 1.46,
      //   the filename() method returned a string.
      //
      // In the Boost filesystem API version 3,
      //   which is the default starting with Boost 1.46,
      //   the filename() method returns a path object.
      //
      // To get a string from the path object,
      //   the native() method must be called.
      //	  hypergraphDir = nbestPath.parent_path().filename()
      //#if BOOST_VERSION >= 104600
      //	    .native()
      //#endif
      //;

      // Hopefully the following compiles under all versions of Boost.
      //
      // If this line gives you compile errors,
      //   contact Lane Schwartz on the Moses mailing list
      m_hypergraphDir = nbestPath.parent_path().string();

    } else {
      stringstream hypergraphDirName;
      hypergraphDirName << boost::filesystem::current_path().string() << "/hypergraph";
      m_hypergraphDir = hypergraphDirName.str();
    }
  }

  if ( ! boost::filesystem::exists(m_hypergraphDir) ) {
    boost::filesystem::create_directory(m_hypergraphDir);
  }

  UTIL_THROW_IF(!boost::filesystem::is_directory(m_hypergraphDir),
                util::Exception, "Cannot output hypergraphs to " << m_hypergraphDir << " because that path exists, but is not a directory");


  ofstream weightsOut;
  stringstream weightsFilename;
  weightsFilename << m_hypergraphDir << "/weights";

  TRACE_ERR("The weights file is " << weightsFilename.str() << "\n");
  weightsOut.open(weightsFilename.str().c_str());
  weightsOut.setf(std::ios::fixed);
  weightsOut.precision(6);
  staticData.GetAllWeights().Save(weightsOut);
  weightsOut.close();
}

template<class M>
void HypergraphOutput<M>::Write(const M& manager) const
{

  stringstream fileName;
  fileName << m_hypergraphDir << "/" << manager.GetSource().GetTranslationId();
  if ( m_appendSuffix ) {
    fileName << "." << m_compression;
  }
  boost::iostreams::filtering_ostream file;

  if ( m_compression == "gz" ) {
    file.push( boost::iostreams::gzip_compressor() );
  } else if ( m_compression == "bz2" ) {
    file.push( boost::iostreams::bzip2_compressor() );
  }

  file.push( boost::iostreams::file_sink(fileName.str(), ios_base::out) );

  if (file.is_complete() && file.good()) {
    file.setf(std::ios::fixed);
    file.precision(m_precision);
    manager.OutputSearchGraphAsHypergraph(file);
    file.flush();
  } else {
    TRACE_ERR("Cannot output hypergraph for line " << manager.GetSource().GetTranslationId()
              << " because the output file " << fileName.str()
              << " is not open or not ready for writing"
              << std::endl);
  }
  file.pop();
}

template class HypergraphOutput<Manager>;
template class HypergraphOutput<ChartManager>;


void ChartSearchGraphWriterMoses::WriteHypos
(const ChartHypothesisCollection& hypos, const map<unsigned, bool> &reachable) const
{

  ChartHypothesisCollection::const_iterator iter;
  for (iter = hypos.begin() ; iter != hypos.end() ; ++iter) {
    ChartHypothesis &mainHypo = **iter;
    if (StaticData::Instance().GetUnprunedSearchGraph() ||
        reachable.find(mainHypo.GetId()) != reachable.end()) {
      (*m_out) << m_lineNumber << " " << mainHypo << endl;
    }

    const ChartArcList *arcList = mainHypo.GetArcList();
    if (arcList) {
      ChartArcList::const_iterator iterArc;
      for (iterArc = arcList->begin(); iterArc != arcList->end(); ++iterArc) {
        const ChartHypothesis &arc = **iterArc;
        if (reachable.find(arc.GetId()) != reachable.end()) {
          (*m_out) << m_lineNumber << " " << arc << endl;
        }
      }
    }
  }

}
void ChartSearchGraphWriterHypergraph::WriteHeader(size_t winners, size_t losers) const
{

  (*m_out) << "# target ||| features ||| source-covered" << endl;
  (*m_out) << winners <<  " " << (winners+losers) << endl;

}

void ChartSearchGraphWriterHypergraph::WriteHypos(const ChartHypothesisCollection& hypos,
    const map<unsigned, bool> &reachable) const
{

  ChartHypothesisCollection::const_iterator iter;
  for (iter = hypos.begin() ; iter != hypos.end() ; ++iter) {
    const ChartHypothesis* mainHypo = *iter;
    if (!StaticData::Instance().GetUnprunedSearchGraph() &&
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
    for (vector<const ChartHypothesis*>::const_iterator ei = edges.begin(); ei != edges.end(); ++ei) {
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

