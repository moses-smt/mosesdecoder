package org.statmt.hg4;

import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import java.util.PriorityQueue;

public class FinishingAgenda extends PriorityQueue<VertexGroup> {

	private static final long serialVersionUID = 12142142L;
	
	Comparator<Vertex> vertexComp;
	HashMap<Map<String,Object>, VertexGroup> groups;
	VertexSignatureCreator sigCreator;
	
	static class VertexGroupCompare implements Comparator<VertexGroup> {

		Comparator<Vertex> vertexComp;
		public VertexGroupCompare(Comparator<Vertex> c) {
			vertexComp = c;
		}
		public int compare(VertexGroup o1, VertexGroup o2) {
			return vertexComp.compare(o1.peek(), o2.peek());
		}
		
	}
	
	public FinishingAgenda(VertexSignatureCreator c,
			Comparator<Vertex> vertexRanker) {
		super(5, new VertexGroupCompare(vertexRanker));
		sigCreator = c;
	}
	
	public void notifyBest(VertexGroup g, Vertex v) {
		this.remove(g);
		this.add(g);
	}
	
	public void add(Vertex v, Map<Vertex, VertexState> v2s) {
		Map<String, Object> sig = sigCreator.signature(v2s.get(v));
		VertexGroup g = groups.get(sig);
		if (g == null) {
			g = new VertexGroup(this, vertexComp, sig);
			groups.put(sig,g);
		}
		g.add(v);
	}
	
}
