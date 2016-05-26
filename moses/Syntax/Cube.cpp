#include "Cube.h"

#include "moses/FF/FFState.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/StaticData.h"

#include "SVertex.h"

namespace Moses
{

namespace Syntax
{

Cube::Cube(const SHyperedgeBundle &bundle)
  : m_bundle(bundle)
{
  // Create the SHyperedge for the 'corner' of the cube.
  std::vector<int> coordinates(bundle.stacks.size()+1, 0);
  SHyperedge *hyperedge = CreateHyperedge(coordinates);
  // Add its coordinates to the set of visited coordinates.
  std::pair<CoordinateSet::iterator, bool> p = m_visited.insert(coordinates);
  const std::vector<int> &storedCoordinates = *p.first;
  // Add the SHyperedge to the queue along with its coordinates (which will be
  // needed for creating its neighbours).
  m_queue.push(QueueItem(hyperedge, &storedCoordinates));
}

Cube::~Cube()
{
  // Delete the SHyperedges belonging to any unpopped items.  Note that the
  // coordinate vectors are not deleted here since they are owned by m_visited
  // (and so will be deleted by its destructor).
  while (!m_queue.empty()) {
    QueueItem item = m_queue.top();
    m_queue.pop();
    // Delete hyperedge and its head (head deletes hyperedge).
    delete item.first->head;  // TODO shared ownership of head vertex?
  }
}

SHyperedge *Cube::Pop()
{
  QueueItem item = m_queue.top();
  m_queue.pop();
  CreateNeighbours(*item.second);
  return item.first;
}

void Cube::CreateNeighbours(const std::vector<int> &coordinates)
{
  // Create a copy of the origin coordinates that will be adjusted for
  // each neighbour.
  std::vector<int> tmpCoordinates(coordinates);

  // Create each neighbour along the vertex stack dimensions.
  for (std::size_t i = 0; i < coordinates.size()-1; ++i) {
    const std::size_t x = coordinates[i];
    if (m_bundle.stacks[i]->size() > x+1) {
      ++tmpCoordinates[i];
      CreateNeighbour(tmpCoordinates);
      --tmpCoordinates[i];
    }
  }
  // Create the neighbour along the translation dimension.
  const std::size_t x = coordinates.back();
  if (m_bundle.translations->GetSize() > x+1) {
    ++tmpCoordinates.back();
    CreateNeighbour(tmpCoordinates);
    --tmpCoordinates.back();
  }
}

void Cube::CreateNeighbour(const std::vector<int> &coordinates)
{
  // Add the coordinates to the set of visited coordinates if not already
  // present.
  std::pair<CoordinateSet::iterator, bool> p = m_visited.insert(coordinates);
  if (!p.second) {
    // We have visited this neighbour before, so there is nothing to do.
    return;
  }
  SHyperedge *hyperedge = CreateHyperedge(coordinates);
  const std::vector<int> &storedCoordinates = *p.first;
  m_queue.push(QueueItem(hyperedge, &storedCoordinates));
}

SHyperedge *Cube::CreateHyperedge(const std::vector<int> &coordinates)
{
  SHyperedge *hyperedge = new SHyperedge();

  SVertex *head = new SVertex();
  head->best = hyperedge;
  head->pvertex = 0;  // FIXME???
  head->states.resize(
    StatefulFeatureFunction::GetStatefulFeatureFunctions().size());
  hyperedge->head = head;

  hyperedge->tail.resize(coordinates.size()-1);
  for (std::size_t i = 0; i < coordinates.size()-1; ++i) {
    boost::shared_ptr<SVertex> pred = (*m_bundle.stacks[i])[coordinates[i]];
    hyperedge->tail[i] = pred.get();
  }

  hyperedge->label.inputWeight = m_bundle.inputWeight;

  hyperedge->label.translation =
    *(m_bundle.translations->begin()+coordinates.back());

  // Calculate feature deltas.

  const StaticData &staticData = StaticData::Instance();

  // compute values of stateless feature functions that were not
  // cached in the translation option-- there is no principled distinction
  const std::vector<const StatelessFeatureFunction*>& sfs =
    StatelessFeatureFunction::GetStatelessFeatureFunctions();
  for (unsigned i = 0; i < sfs.size(); ++i) {
    if (!staticData.IsFeatureFunctionIgnored(*sfs[i])) {
      sfs[i]->EvaluateWhenApplied(*hyperedge, &hyperedge->label.deltas);
    }
  }

  const std::vector<const StatefulFeatureFunction*>& ffs =
    StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i) {
    if (!staticData.IsFeatureFunctionIgnored(*ffs[i])) {
      head->states[i] =
        ffs[i]->EvaluateWhenApplied(*hyperedge, i, &hyperedge->label.deltas);
    }
  }

  // Calculate future score.

  hyperedge->label.futureScore =
    hyperedge->label.translation->GetScoreBreakdown().GetWeightedScore();

  hyperedge->label.futureScore += hyperedge->label.deltas.GetWeightedScore();

  for (std::vector<SVertex*>::const_iterator p = hyperedge->tail.begin();
       p != hyperedge->tail.end(); ++p) {
    const SVertex *pred = *p;
    if (pred->best) {
      hyperedge->label.futureScore += pred->best->label.futureScore;
    }
  }

  return hyperedge;
}

}  // Syntax
}  // Moses
