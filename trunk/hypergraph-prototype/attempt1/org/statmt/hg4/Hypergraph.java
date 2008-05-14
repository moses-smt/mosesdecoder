package org.statmt.hg4;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Queue;
import java.util.Set;

public class Hypergraph {

        Set<Vertex> sinkVertices; //
        Set<Vertex> sourceVertices; //no children

        /**
         * active and passive edges
         *
         * passive edge (rule,phrase pair, introduced at start)
         * active edge (hypothesis)
         * parse: 2 adgenda/lists
         * finishing agenda
         * put all rules in passive, one empty passive edge
         * @param vertex
         */

        public void addNode(Vertex vertex) {
                if (!vertex.hasChildren()) sourceVertices.add(vertex);
                sinkVertices.add(vertex);
        }

        /**
         * Assumes vertex and edge's children are already in graph
         * @param vertex
         * @param edge
         */
        public void addLink(Vertex vertex, Hyperarc arc) {
                vertex.addArc(arc);
                for(Vertex child : arc.getTail()) {
                    sinkVertices.remove(child);
            }
            sourceVertices.remove(vertex);
    }

        static class Phrase {
        	String[] d;
        	public Phrase(String ds) {
        		d = ds.split("\\s+");
        	}
        	@Override
        	public int hashCode() { return d.hashCode() * 37; }
        	@Override
        	public boolean equals(Object r) {
        		Phrase p = (Phrase)r;
        		if (p.d.length != this.d.length) return false;
        		for (int i =0; i<d.length; i++)
        			if (d[i].compareTo(p.d[i]) != 0) return false;
        		return true;
        	}
        	public Phrase slice(int x, int y) {
        		return null;
        	}
        }
        
        static class PTable extends HashMap<Phrase,ArrayList<Phrase>> {
        	public ArrayList<ArrayList<ArrayList<String[]>>> buildLattice(String sentence) {
        		String[] words = sentence.split("\\s+");
        		return null;
        	}
        }
    public static void main(String[] args) {
    	Vertex source = new Vertex(null);
    	
 /*   	PPair p1 = new PPair("guten", "good");
    	PPair p2 = new PPair("tag", "day");
    	PPair p3 = new PPair("tag", "hi");
    	PPair p4 = new PPair("guten", "well");
    	PPair p5 = new PPair("guten tag", "good day");
    	PPair p6 = new PPair("guten tag", "hello");*/
    	PTable pt = new PTable();
    }
}