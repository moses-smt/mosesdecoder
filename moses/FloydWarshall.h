#ifndef moses_FloydWarshall_h
#define moses_FloydWarshall_h

#include <vector>

/**
 * Floyd-Warshall all-pairs shortest path algorithm
 * See CLR (1990). Introduction to Algorithms, p. 558-565
 */
void floyd_warshall(const std::vector<std::vector<bool> >& edges, std::vector<std::vector<int> >& distances);

#endif
