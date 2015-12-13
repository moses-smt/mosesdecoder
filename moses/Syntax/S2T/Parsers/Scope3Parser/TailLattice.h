#pragma once

#include <utility>
#include <vector>

namespace Moses
{
namespace Syntax
{
namespace S2T
{

/* Lattice in which a full path corresponds to the tail of a PHyperedge.
 * For an entry x[i][j][k][l] in a TailLattice x:
 *
 *  i = offset from start of rule pattern
 *
 *  j = index of gap + 1 (zero indicates a terminal, otherwise the index is
 *      zero-based from the left of the rule pattern)
 *
 *  k = arc width
 *
 *  l = label index (zero for terminals, otherwise as in RuleTrieScope3::Node)
 */
typedef std::vector<
std::vector<
std::vector<
std::vector<const PVertex *> > > > TailLattice;

}  // namespace S2T
}  // namespace Syntax
}  // namespace Moses
