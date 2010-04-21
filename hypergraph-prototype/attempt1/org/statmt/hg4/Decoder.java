package org.statmt.hg4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
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
	
	static class PBRanker implements Comparator<Vertex> {
		Map<Vertex,VertexState> v2s;
		public PBRanker(Map<Vertex,VertexState> v2s) {
			this.v2s = v2s;
		}
		public int compare(Vertex o1, Vertex o2) {
			float f1 = (Float)v2s.get(o1).get("PROB");
			float f2 = (Float)v2s.get(o2).get("PROB");
			if (f1 == f2) return 0;
			if (f1 < f2) return -1; else return 1;
		}
	}
		
	public static Hypergraph parse(Phrase sentence,
			SearchStrategy ss,
			Hypergraph dg,
			Map<Vertex,VertexState> v2smap) {
		ExplorationAgenda expAgenda = new ExplorationAgenda();
		Map<VertexState, Vertex> s2vmap =
			new HashMap<VertexState, Vertex>();
		VertexSignatureCreator sigc = new DumbPBSignature();
		Vertex start = new Vertex(null);
		VertexState sst = new VertexState();
		sst.put("PROB", new Float(0.0f));
		sst.put("ACTIVE?", new Boolean(true));
		sst.put("COVERAGE", new Coverage(sentence.size()));
		v2smap.put(start, sst);
		s2vmap.put(sst, start);
		dg.addNode(start);
		PBRanker pbr = new PBRanker(v2smap);
		FinishingAgenda fa =
			new FinishingAgenda(sigc, pbr);
		fa.add(start, v2smap);
		while (!fa.isEmpty() || !expAgenda.isEmpty()) {
			while (!expAgenda.isEmpty()) {
				// p is a "traversal"
				APPair p = expAgenda.nextVertex();
				System.out.println("ExpAgenda: processing traversal " + p);
				Hyperarc ha = new Hyperarc(p.asTail());
			
				// "edge" discovery
				VertexState newState = ss.applyFundamentalRule(ha, v2smap);
				Vertex v = s2vmap.get(newState);
				if (v == null) {
					v = new Vertex(new ArrayList<Hyperarc>());
					dg.addNode(v);
					v2smap.put(v, newState);
				}
				dg.addLink(v, ha);
				System.out.println("ADDING " + newState);
				fa.add(v, v2smap);
			}
			if (fa.peek() != null) {
				VertexGroup vg = fa.poll();
				System.out.println("FA: processing finishing vertex (group) " + vg);
				VertexSignature sig = vg.getSignature();
				System.out.println("  sig=" + sig);
				if (vg.isActive()) {
					Collection<Vertex> p = ss.retrieveCombinableVertices(sig, dg, sentence);
					ss.generateTraversals(vg, p, expAgenda);
				} else {
					System.out.println("NOACT!!!");
				}
			}
		}
		return dg;
	}
	
	public static void main(String[] args) {
		Phrase sentence = new Phrase("guten tag");
	   	PTable pt = new PTable();
     	pt.add(new Phrase("guten"), new Phrase("well"));
        pt.add(new Phrase("guten"), new Phrase("good"));
    	pt.add(new Phrase("tag"), new Phrase("day"));
    	pt.add(new Phrase("tag"), new Phrase("hi"));
    	pt.add(new Phrase("guten tag"), new Phrase("hello"));
    	pt.add(new Phrase("guten tag"), new Phrase("good day"));
    	ArrayList<ArrayList<ArrayList<Phrase>>> lattice =
    		pt.buildLattice(sentence);
    	Map<Vertex, VertexState> v2s = new HashMap<Vertex, VertexState>();
    	Vertex source = new Vertex(null);
    	Hypergraph hg = new Hypergraph();
    	hg.addNode(source);

		parse(sentence,
			new DumbPBSearchPolicy(sentence, lattice, hg, v2s),
			hg, v2s);
	}
}
