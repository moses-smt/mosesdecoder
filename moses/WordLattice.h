#ifndef moses_WordLattice_h
#define moses_WordLattice_h

#include <iostream>
#include <vector>
#include "ConfusionNet.h"
#include "PCNTools.h"

namespace Moses
{

/** An input to the decoder that represent a word lattice.
 *  @todo why is this inherited from confusion net?
 */
class WordLattice: public ConfusionNet
{
  friend std::ostream& operator<<(std::ostream &out, const WordLattice &obj);
private:
  std::vector<std::vector<size_t> > next_nodes;
  std::vector<std::vector<int> > distances;

public:
  WordLattice();
  size_t GetColumnIncrement(size_t ic, size_t j) const;
  void Print(std::ostream&) const;
  /** Get shortest path between two nodes
   */
  virtual int ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) const;
  // is it possible to get from the edge of the previous word range to the current word range
  virtual bool CanIGetFromAToB(size_t start, size_t end) const;

  /** Given a lattice represented using the PCN::CN data type (topologically sorted agency list
   * representation), initialize the WordLattice object
   */
  int InitializeFromPCNDataType(const PCN::CN& cn, const std::vector<FactorType>& factorOrder, const std::string& debug_line = "");
  /** Read from PLF format (1 lattice per line)
   */
  int Read(std::istream& in,const std::vector<FactorType>& factorOrder);

  /** Convert internal representation into an edge matrix
   * @note edges[1][2] means there is an edge from 1 to 2
   */
  void GetAsEdgeMatrix(std::vector<std::vector<bool> >& edges) const;

  const std::vector<size_t> &GetNextNodes(size_t pos) const {
    return next_nodes[pos];
  }

  TranslationOptionCollection *CreateTranslationOptionCollection() const;
};

}

#endif
