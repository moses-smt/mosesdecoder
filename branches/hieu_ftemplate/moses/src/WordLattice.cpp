#include "StaticData.h"
#include "WordLattice.h"
#include "PCNTools.h"
#include "Util.h"
#include "FloydWarshall.h"

WordLattice::WordLattice() {}

size_t WordLattice::GetColumnIncrement(size_t i, size_t j) const
{
	return next_nodes[i][j];
}

void WordLattice::Print(std::ostream& out) const {
	out<<"word lattice: "<<data.size()<<"\n";
	for(size_t i=0;i<data.size();++i) {
		out<<i<<" -- ";
		for(size_t j=0;j<data[i].size();++j)
			out<<"("<<data[i][j].first.ToString()<<", "<<data[i][j].second<<", " << GetColumnIncrement(i,j) << ") ";
		out<<"\n";
	}
	out<<"\n\n";
}

int WordLattice::Read(std::istream& in,const std::vector<FactorType>& factorOrder)
{
	Clear();
	std::string line;
	if(!getline(in,line)) return 0;
	std::map<std::string, std::string> meta=ProcessAndStripSGML(line);
	if (meta.find("id") != meta.end()) { this->SetTranslationId(atol(meta["id"].c_str())); }
	PCN::CN cn = PCN::parsePCN(line);
	data.resize(cn.size());
	next_nodes.resize(cn.size());
	for(size_t i=0;i<cn.size();++i) {
		PCN::CNCol& col = cn[i];
		if (col.empty()) return false;
		data[i].resize(col.size());
		next_nodes[i].resize(col.size());
		for (size_t j=0;j<col.size();++j) {
			PCN::CNAlt& alt = col[j];
			if (alt.first.second < 0.0f) { TRACE_ERR("WARN: neg probability: " << alt.first.second); alt.first.second = 0.0f; }
			if (alt.first.second > 1.0f) { TRACE_ERR("WARN: probability > 1: " << alt.first.second); alt.first.second = 1.0f; }
			data[i][j].second = std::max(static_cast<float>(log(alt.first.second)), LOWEST_SCORE);
			String2Word(alt.first.first,data[i][j].first,factorOrder);
			next_nodes[i][j] = alt.second;
		}
	}
	if (!cn.empty()) {
		std::vector<std::vector<bool> > edges(0);
		this->GetAsEdgeMatrix(edges);
		floyd_warshall(edges,distances);

		IFVERBOSE(2) {
			TRACE_ERR("Shortest paths:\n");
			for (size_t i=0; i<edges.size(); ++i) {
				for (size_t j=0; j<edges.size(); ++j) {
					int d = distances[i][j];
					if (d > 99999) { d=-1; }
					TRACE_ERR("\t" << d);
				}
				TRACE_ERR("\n");
			}
		}
	}
	return !cn.empty();
}

void WordLattice::GetAsEdgeMatrix(std::vector<std::vector<bool> >& edges) const
{
  edges.resize(data.size()+1,std::vector<bool>(data.size()+1, false));
  for (size_t i=0;i<data.size();++i) {
    for (size_t j=0;j<data[i].size(); ++j) {
      edges[i][i+next_nodes[i][j]] = true;
    }
  }
}

int WordLattice::ComputeDistortionDistance(const WordsRange& prev, const WordsRange& current) const
{
  if (prev.GetStartPos() == NOT_FOUND) {
    return distances[0][current.GetStartPos()+1] - 1;
  } else if (prev.GetEndPos() > current.GetStartPos()) {
    return distances[current.GetStartPos()][prev.GetEndPos()+1];
  } else {
    return distances[prev.GetEndPos()+1][current.GetStartPos()+1] - 1;
  }
}

bool WordLattice::CanIGetFromAToB(size_t start, size_t end) const
{
//  std::cerr << "CanIgetFromAToB(" << start << "," << end << ")=" << distances[start][end] << std::endl;
  return distances[start][end] < 100000;
}

