#include "WordLattice.h"
#include "PCNTools.h"

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
	return !cn.empty();
}

