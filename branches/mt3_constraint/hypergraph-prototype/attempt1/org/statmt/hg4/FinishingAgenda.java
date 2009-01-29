package org.statmt.hg4;

import java.util.Comparator;
import java.util.HashMap;
import java.util.Map;
import java.util.PriorityQueue;

public class FinishingAgenda extends PriorityQueue<VertexGroup> {

	private static final long serialVersionUID = 12142142L;
	
	Comparator<Vertex> vertexComp;
	HashMap<VertexSignature, VertexGroup> groups =
		new HashMap<VertexSignature, VertexGroup>();
	VertexSignatureCreator sigCreator;
	
	static class VertexGroupCompare implements Comparator<VertexGroup> {

		Comparator<Vertex> vertexComp;
		public VertexGroupCompare(Comparator<Vertex> c) {
			vertexComp = c;
		}
		public int compare(VertexGroup o1, VertexGroup o2) {
			int vc = vertexComp.compare(o1.peek(), o2.peek());
			if (vc == 0) {
				return o1.getSignature().compareTo(o2.getSignature());
			} else return vc;
		}
		
	}
	
	public FinishingAgenda(VertexSignatureCreator c,
			Comparator<Vertex> vertexRanker) {
		super(5, new VertexGroupCompare(vertexRanker));
		vertexComp = vertexRanker;
		sigCreator = c;
	}
	
	public void notifyBest(VertexGroup g, Vertex v) {
		this.remove(g);
		this.add(g);
		System.out.println("Trying to add " + g + "\tv="+v);
	}
	
	public void add(Vertex v, Map<Vertex, VertexState> v2s) {
		VertexSignature sig = sigCreator.signature(v2s.get(v));
		VertexGroup g = groups.get(sig);
		//System.out.println("@@@FA.add(" + sig +")\tHC="+sig.hashCode() + "  g=" + g);
		if (g == null) {
			g = new VertexGroup(this, vertexComp, sig);
			groups.put(sig,g);
		}
		g.add(v);
		System.out.println("FINSIHING.SIZE=" + this.size() + "\tVG.SIZE="+g.size() );
	}
	
	@Override
	public VertexGroup poll() {
		VertexGroup g = super.poll();
		groups.remove(g.getSignature());
		return g;
	}
	
}
