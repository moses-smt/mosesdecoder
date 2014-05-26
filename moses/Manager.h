// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

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

#ifndef moses_Manager_h
#define moses_Manager_h

#include <vector>
#include <list>
#include "InputType.h"
#include "Hypothesis.h"
#include "StaticData.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "TrellisPathList.h"
#include "SquareMatrix.h"
#include "WordsBitmap.h"
#include "Search.h"
#include "SearchCubePruning.h"

namespace Moses
{

class SentenceStats;
class TrellisPath;
class TranslationOptionCollection;

/** Used to output the search graph */
struct SearchGraphNode {
  const Hypothesis* hypo;
  const Hypothesis* recombinationHypo;
  int forward;
  double fscore;

  SearchGraphNode(const Hypothesis* theHypo,
                  const Hypothesis* theRecombinationHypo,
                  int theForward,
                  double theFscore) :
    hypo(theHypo), recombinationHypo(theRecombinationHypo),
    forward(theForward), fscore(theFscore) {}

  bool operator<(const SearchGraphNode& sgn) const {
    return this->hypo->GetId() < sgn.hypo->GetId();
  }

};

/** The Manager class implements a stack decoding algorithm for phrase-based decoding
 * Hypotheses are organized in stacks. One stack contains all hypothesis that have
 * the same number of foreign words translated.  The data structure for hypothesis
 * stacks is the class HypothesisStack. The data structure for a hypothesis
 * is the class Hypothesis.
 *
 * The main decoder loop in the function ProcessSentence() consists of the steps:
 * - Create the list of possible translation options. In phrase-based decoding
 *   (and also the first mapping step in the factored model) is a phrase translation
 *   from the source to the target. Given a specific input sentence, only a limited
 *   number of phrase translation can be applied. For efficient lookup of the
 *   translation options later, these options are first collected in the function
 *   CreateTranslationOption (for more information check the class
 *   TranslationOptionCollection)
 * - Create initial hypothesis: Hypothesis stack 0 contains only one empty hypothesis.
 * - Going through stacks 0 ... (sentence_length-1):
 *   - The stack is pruned to the maximum size
 *   - Going through all hypotheses in the stack
 *     - Each hypothesis is expanded by ProcessOneHypothesis()
 *     - Expansion means applying a translation option to the hypothesis to create
 *       new hypotheses
 *     - What translation options may be applied depends on reordering limits and
 *       overlap with already translated words
 *     - With a applicable translation option and a hypothesis at hand, a new
 *       hypothesis can be created in ExpandHypothesis()
 *     - New hypothesis are either discarded (because they are too bad), added to
 *       the appropriate stack, or re-combined with existing hypotheses
 **/

class Manager
{
  Manager();
  Manager(Manager const&);
  void operator=(Manager const&);
private:

  // Helper functions to output search graph in HTK standard lattice format
  void OutputFeatureWeightsForSLF(std::ostream &outputSearchGraphStream) const;
  size_t OutputFeatureWeightsForSLF(size_t index, const FeatureFunction* ff, std::ostream &outputSearchGraphStream) const;
  void OutputFeatureValuesForSLF(const Hypothesis* hypo, bool zeros, std::ostream &outputSearchGraphStream) const;
  size_t OutputFeatureValuesForSLF(size_t index, bool zeros, const Hypothesis* hypo, const FeatureFunction* ff, std::ostream &outputSearchGraphStream) const;

  // Helper functions to output search graph in the hypergraph format of Kenneth Heafield's lazy hypergraph decoder
  void OutputFeatureValuesForHypergraph(const Hypothesis* hypo, std::ostream &outputSearchGraphStream) const;
  size_t OutputFeatureValuesForHypergraph(size_t index, const Hypothesis* hypo, const FeatureFunction* ff, std::ostream &outputSearchGraphStream) const;


protected:
  // data
//	InputType const& m_source; /**< source sentence to be translated */
  TranslationOptionCollection *m_transOptColl; /**< pre-computed list of translation options for the phrases in this sentence */
  Search *m_search;

  HypothesisStack* actual_hypoStack; /**actual (full expanded) stack of hypotheses*/
  size_t interrupted_flag;
  std::auto_ptr<SentenceStats> m_sentenceStats;
  int m_hypoId; //used to number the hypos as they are created.
  size_t m_lineNumber;

  void GetConnectedGraph(
    std::map< int, bool >* pConnected,
    std::vector< const Hypothesis* >* pConnectedList) const;
  void GetWinnerConnectedGraph(
    std::map< int, bool >* pConnected,
    std::vector< const Hypothesis* >* pConnectedList) const;


public:
  InputType const& m_source; /**< source sentence to be translated */
  Manager(size_t lineNumber, InputType const& source, SearchAlgorithm searchAlgorithm);
  ~Manager();
  const  TranslationOptionCollection* getSntTranslationOptions();

  void ProcessSentence();
  const Hypothesis *GetBestHypothesis() const;
  const Hypothesis *GetActualBestHypothesis() const;
  void CalcNBest(size_t count, TrellisPathList &ret,bool onlyDistinct=0) const;
  void CalcLatticeSamples(size_t count, TrellisPathList &ret) const;
  void PrintAllDerivations(long translationId, std::ostream& outputStream) const;
  void printDivergentHypothesis(long translationId, const Hypothesis* hypo, const std::vector <const TargetPhrase*> & remainingPhrases, float remainingScore , std::ostream& outputStream) const;
  void printThisHypothesis(long translationId, const Hypothesis* hypo, const std::vector <const TargetPhrase* > & remainingPhrases, float remainingScore , std::ostream& outputStream) const;
  void GetOutputLanguageModelOrder( std::ostream &out, const Hypothesis *hypo );
  void GetWordGraph(long translationId, std::ostream &outputWordGraphStream) const;
  int GetNextHypoId();
#ifdef HAVE_PROTOBUF
  void SerializeSearchGraphPB(long translationId, std::ostream& outputStream) const;
#endif

  void OutputSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const;
  void OutputSearchGraphAsSLF(long translationId, std::ostream &outputSearchGraphStream) const;
  void OutputSearchGraphAsHypergraph(long translationId, std::ostream &outputSearchGraphStream) const;
  void GetSearchGraph(std::vector<SearchGraphNode>& searchGraph) const;
  const InputType& GetSource() const {
    return m_source;
  }

  /***
   * to be called after processing a sentence (which may consist of more than just calling ProcessSentence() )
   */
  void CalcDecoderStatistics() const;
  void ResetSentenceStats(const InputType& source);
  SentenceStats& GetSentenceStats() const;

  /***
   *For Lattice MBR
  */
  void GetForwardBackwardSearchGraph(std::map< int, bool >* pConnected,
                                     std::vector< const Hypothesis* >* pConnectedList, std::map < const Hypothesis*, std::set < const Hypothesis* > >* pOutgoingHyps, std::vector< float>* pFwdBwdScores) const;

};

}
#endif
