package org.statmt.hg4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Dictionary;
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
	
	static class PBCandidateGenerator extends PassiveCandidateGenerator {
		ArrayList<ArrayList<Vertex>> lattice;
		public PBCandidateGenerator(ArrayList<ArrayList<ArrayList<Phrase>>> l, Hypergraph derivation) {
			assert(derivation.sourceVertices.size() == 1);
			Vertex v = derivation.sourceVertices.iterator().next();
			lattice = new ArrayList<ArrayList<ArrayList<Vertex>>>();
			for (ArrayList<ArrayList<Phrase>> cols : l) {
				lattice.add(new ArrayList<ArrayList<Vertex>>()));
			}
		}
		public Collection<Vertex> retrieveCominableNodesForActiveVertex(Vertex activeVertex,
				Map<String, Object> state, Phrase sentence) {
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

	public static void main(String[] args) {
		SGrammar g = new SGrammar();
		g.addRule("[X]", "der [X1] man", "the [X1] man");
		g.addRule("[X]", "der", "the");
	}
}
