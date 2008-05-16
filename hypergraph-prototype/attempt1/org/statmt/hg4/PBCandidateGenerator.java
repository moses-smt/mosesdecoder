package org.statmt.hg4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;

public class PBCandidateGenerator extends CandidateGenerator {

	ArrayList<ArrayList<Vertex>> lattice;
	public PBCandidateGenerator(ArrayList<ArrayList<ArrayList<Phrase>>> l, Hypergraph derivation) {
		assert(derivation.sourceVertices.size() == 1);
		Vertex v = derivation.sourceVertices.iterator().next();
		lattice = new ArrayList<ArrayList<ArrayList<Vertex>>>();
		for (ArrayList<ArrayList<Phrase>> cols : l) {
			lattice.add(new ArrayList<ArrayList<Vertex>>()));
		}
	}

	@Override
	public Collection<Vertex> retrieveCominableNodesForActiveVertex(
			Vertex activeVertex, Map<String, Object> vertexState,
			Phrase sentence) {
		int c = (Integer)state.get("COVERAGE");
		ArrayList<Vertex> transOpts = new ArrayList<Vertex>();
		for (int e = c; e < sentence.size(); e++) {
			lattice.get(c);
			ArrayList<Vertex> opts = lattice.get(c).get(e-c);
			transOpts.addAll(opts);
		}
		return transOpts;
	}

}
