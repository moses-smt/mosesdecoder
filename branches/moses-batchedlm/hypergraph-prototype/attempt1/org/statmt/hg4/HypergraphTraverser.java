package org.statmt.hg4;

public abstract class HypergraphTraverser {

	/**
	 * Traverse a hypergraph.  The implementer should pick some Hyperarc
	 * that in the backward star of vertex, and recursively call traverse
	 * on its tail.  The values returned are ordered as an Object[], which
	 * can then be passed to v.visit for the vertex.
	 * 
	 * @param vertex
	 * @param v
	 * @return
	 */
	public abstract Object traverse(Vertex vertex, HyperarcVisitor v);
}
