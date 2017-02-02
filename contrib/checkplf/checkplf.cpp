#include <iostream>

#include "PCNTools.h"

using namespace std;

int main()
{
  cerr << "Reading PLF from STDIN...\n";
  string line;
  int lc = 0;
  double num_paths = 0;
  double num_nodes = 0;
  double num_edges = 0;
  while(cin) {
    getline(cin,line);
    if (line.empty()) continue;
    ++lc;
    PCN::CN plf = PCN::parsePCN(line);
    vector<double> alphas(plf.size() + 1, 0.0);
    num_nodes += plf.size();
    alphas[0] = 1.0;
    for (unsigned node = 0; node < plf.size(); ++node) {
      const PCN::CNCol& edges = plf[node];
      const double alpha = alphas[node];
      if (alpha < 1.0) {
        cerr << "Line " << lc << ": unreachable node at column position " << (node+1) << endl;
        return 1;
      }
      num_edges += edges.size();
      for (unsigned j = 0; j < edges.size(); ++j) {
        const PCN::CNAlt& edge = edges[j];
        size_t head = edge.m_next + node;
        const string& label = edge.m_word;
        if (head <= node) {
          cerr << "Line " << lc << ": cycle detected at column position " << (node+1) << ", edge label = '" << label << "'" << endl;
          return 1;
        }
        if (head >= alphas.size()) {
          cerr << "Line " << lc << ": edge goes beyond goal node at column position " << (node+1) << ", edge label = '" << label << "'" << endl;
          cerr << "  Goal node expected at position " << alphas.size() << ", but edge references a node at position " << head << endl;
          return 1;
        }
        alphas[head] += alpha;
      }
    }
    if (alphas.back() < 1.0) {
      cerr << "Line " << lc << ": there appears to be no path to the goal" << endl;
      return 1;
    }
    num_paths += alphas.back();
  }
  cerr << "PLF format appears to be correct.\nSTATISTICS:\n";
  cerr << "       Number of lattices: " << lc << endl;
  cerr << "    Total number of nodes: " << num_nodes << endl;
  cerr << "    Total number of edges: " << num_edges << endl;
  cerr << "          Average density: " << (num_edges / num_nodes) << " edges/node\n";
  cerr << "    Total number of paths: " << num_paths << endl;
  cerr << "  Average number of paths: " << (num_paths / lc) << endl;
  return 0;
}

