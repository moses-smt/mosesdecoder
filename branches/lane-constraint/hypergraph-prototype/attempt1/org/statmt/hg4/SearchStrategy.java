package org.statmt.hg4;

import java.util.Collection;
import java.util.Map;

public abstract class SearchStrategy {
	
	public abstract Collection<Vertex> retrieveCombinableVertices(
			VertexSignature signatureOfVertex,
			Hypergraph derivationGraph,
			Phrase sentence);
	
	public abstract VertexState applyFundamentalRule(
			Hyperarc ha,
			Map<Vertex, VertexState> v2s);

	public abstract void generateTraversals(
			VertexGroup activeEdges,
			Collection<Vertex> compatibleKnownThings,
			ExplorationAgenda agenda);

}
