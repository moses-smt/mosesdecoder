package org.statmt.hg4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;

public class Decoder {

	static class GNode {
		HashMap<String, GNode> edges =
			new HashMap<String, GNode>();
		HashMap<String, ArrayList<Object>> data =
			new HashMap<String,ArrayList<Object>>();
		public void addEdge(String word, GNode node) {
			edges.put(word, node);
		}
		public GNode getOrAddNode(String edgeName) {
			GNode x = edges.get(edgeName);
			if (x == null) {
				x = new GNode();
				edges.put(edgeName, x);
			}
			return x;
		}
		public void addData(String lhs, Object o) {
			ArrayList<Object> d = data.get(lhs);
			if (d == null) {
				d = new ArrayList<Object>();
				data.put(lhs, d);
			}
			d.add(o);
		}
		public GNode followEdge(String word) {
			return edges.get(word);
		}
	}
	static class SGrammar {
		GNode root = new GNode();
		public void addRule(String lhs, String rhs, Object data) {
			String[] its = rhs.split("\\s+");
			GNode cur = root;
			for (String word: its) {
				cur = cur.getOrAddNode(word);
			}
			cur.addData(lhs, data);
		}
		
		public GNode getRoot() { return root; } 
	}
	
	public static Hypergraph parse(Phrase sentence,
			ExplorationAgenda expAgenda,
			SearchStrategy cg,
			FundamentalRuleFunctor fr,
			Hypergraph dg) {
		Map<Vertex,VertexState> v2smap =
			new HashMap<Vertex, VertexState>();
		Map<VertexState, Vertex> s2vmap =
			new HashMap<VertexState, Vertex>();
		VertexSignatureCreator sigc = null;
		FinishingAgenda fa = new FinishingAgenda(sigc, null);
		while (!fa.isEmpty() && !expAgenda.isEmpty()) {
			while (!expAgenda.isEmpty()) {
				// p is a "traversal"
				APPair p = expAgenda.nextVertex();
				Hyperarc ha = new Hyperarc(p.asTail());
			
				// "edge" discovery
				VertexState newState = fr.apply(ha, v2smap);
				Vertex v = s2vmap.get(newState);
				if (v == null) {
					v = new Vertex(new ArrayList<Hyperarc>());
					dg.addNode(v);
				}
				dg.addLink(v, ha);
			}
			VertexGroup vg = fa.peek();
			Map<String, Object> sig = vg.getSignature();
			Collection<Vertex> p = cg.retrieveCombinableVertices(sig, dg, sentence);
			cg.generateTraversals(vg, p, expAgenda);
		}
		return dg;
	}
	
	public static void main(String[] args) {
	}
}
