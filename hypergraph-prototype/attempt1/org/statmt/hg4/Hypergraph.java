package org.statmt.hg4;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class Hypergraph {

        Set<Vertex> sinkVertices = new HashSet<Vertex>(); //
        Set<Vertex> sourceVertices = new HashSet<Vertex>(); //no children

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
        
        public String toString() {
        	String x = "HG\n  SRC="+sourceVertices + "\n SINK="+sinkVertices +"\n";
        	for (Vertex v : sinkVertices) {
        		x += "\n    V="+v;
        	}
        	return x;
        }
        
    static class Rule extends Phrase {
    	public Rule(Phrase p) { super(p.d); }
    	
    	public String getTarget() {
    		String r = "";
    		for (int i =0; i<d.length; i++)
    			r+=(i == 0 ? d[i] : " " + d[i]);
    		return r;
    	}
    	
    }
    
    static class StringifyPB extends HyperarcVisitor {
    	HashMap<Hyperarc, Rule> map;
    	public StringifyPB(HashMap<Hyperarc, Rule> m) { map = m; }
    	public Object transition(Hyperarc h, Object[] children) {
    		Rule r = map.get(h);
    		if (children.length > 0 && children[0] != null) {
    			return (String)children[0] + " " + r.getTarget();
    		} else return r.getTarget();
    	}
    }

    static class StringifyPBWithPhraseMarkup extends HyperarcVisitor {
    	HashMap<Hyperarc, Rule> map;
    	public StringifyPBWithPhraseMarkup(HashMap<Hyperarc, Rule> m) { map = m; }
    	public Object transition(Hyperarc h, Object[] children) {
    		Rule r = map.get(h);
    		if (children.length > 0 && children[0] != null) {
    			return (String)children[0] + "|" + r.getTarget() + "|";
    		} else return "|" + r.getTarget() + "|";
    	}
    }
    
    static class ViterbiTraverser extends HypergraphTraverser {
    	public Object traverse(Vertex vertex, HyperarcVisitor visitor) {
    		if (vertex.incomingArcs == null) {
    			return null;
    		}
       		// get best (first == best)
    		Hyperarc arc = vertex.incomingArcs.get(0);
    		List<Vertex> vs = arc.getTail();
    		Object[] children = null;
    		if (vs != null) {
    			children = new Object[vs.size()];
    			int c = 0;
    			for (Vertex vc : vs) {
    				children[c] = traverse(vc, visitor);
    				c++;
    			}
    		}
    		return visitor.transition(arc, children);
    	}
    }
    
    public static Object viterbi(Hypergraph g, HyperarcVisitor v) {
    	HypergraphTraverser t = new ViterbiTraverser();
    	return t.traverse(g.sinkVertices.iterator().next(), v);
    }
              
    public static void main(String[] args) {
    	PTable pt = new PTable();
     	pt.add(new Phrase("guten"), new Phrase("well"));
        pt.add(new Phrase("guten"), new Phrase("good"));
    	pt.add(new Phrase("tag"), new Phrase("day"));
    	pt.add(new Phrase("tag"), new Phrase("hi"));
    	pt.add(new Phrase("guten tag"), new Phrase("hello"));
    	pt.add(new Phrase("guten tag"), new Phrase("good day"));
    	Phrase sent = new Phrase("guten tag");
    	HashMap<Hyperarc, Rule> rules =
    		new HashMap<Hyperarc, Rule>();
    	ArrayList<ArrayList<ArrayList<Phrase>>> lattice =
    		pt.buildLattice(sent);
    	Vertex source = new Vertex(null);
    	ArrayList<Vertex>[] stacks = new ArrayList[sent.size() + 1];
    	for (int i = 0; i < stacks.length; i++)
    		stacks[i] = new ArrayList<Vertex>();
    	stacks[0].add(source);
    	Hypergraph hg = new Hypergraph();
    	hg.addNode(source);
    	for (int i = 0; i < sent.size(); i++) {
    		ArrayList<Vertex> shs = stacks[i];
    		for (Vertex prevHyp : shs) {
    			ArrayList<Vertex> tail = new ArrayList<Vertex>(1);
    			tail.add(prevHyp);
    			int c = i;
    			for (int e = c; e < sent.size(); e++) {
    				lattice.get(c);
    				ArrayList<Phrase> opts = lattice.get(c).get(e-c);
    				
    				for (Phrase opt : opts) {
    					Rule r = new Rule(opt);
        				Hyperarc ext = new
    						Hyperarc(tail);
        				int targetStack = e + 1;
        				ArrayList<Vertex> targetS =
        					stacks[targetStack];
        				if (targetS.size() == 0) {
        					targetS.add(new Vertex(new ArrayList<Hyperarc>()));
        					hg.addNode(targetS.get(0));
        				}
        				hg.addLink(targetS.get(0), ext);
        				rules.put(ext, r);
        					
        				System.out.println(ext + " r="+r + "  dest=" + targetStack);
    				}
    			}
    		}
    	}
    	System.out.println(viterbi(hg, new StringifyPB(rules)));	
    	System.out.println(viterbi(hg, new StringifyPBWithPhraseMarkup(rules)));	
    }
}