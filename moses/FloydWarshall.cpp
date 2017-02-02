#include "util/exception.hh"
#include <climits>
#include <vector>

#define MAX_DIST (INT_MAX / 2)

//#include "FloydWarshall.h"

using namespace std;

// All-pairs shortest path algorithm
void floyd_warshall(const std::vector<std::vector<bool> >& edges, std::vector<std::vector<int> >& dist)
{
  UTIL_THROW_IF2(edges.size() != edges.front().size(), "Error");
  dist.clear();
  dist.resize(edges.size(), std::vector<int>(edges.size(), 0));

  size_t num_edges = edges.size();

  for (size_t i=0; i<num_edges; ++i) {
    for (size_t j=0; j<num_edges; ++j) {
      if (edges[i][j])
        dist[i][j] = 1;
      else
        dist[i][j] = MAX_DIST;
      if (i == j) dist[i][j] = MAX_DIST;
    }
  }

  for (size_t k=0; k<num_edges; ++k)
    for (size_t i=0; i<num_edges; ++i)
      for (size_t j=0; j<num_edges; ++j)
        if (dist[i][j] > (dist[i][k] + dist[k][j]))
          dist[i][j] = dist[i][k] + dist[k][j];
}

