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
	

	public static void main(String[] args) {
		SGrammar g = new SGrammar();
		g.addRule("[X]", "der [X1] man", "the [X1] man");
		g.addRule("[X]", "der", "the");
	}
}
