package org.statmt.hg4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;
import java.util.HashMap;

import org.statmt.hg4.Hypergraph.Rule;

public class PBCandidateGenerator  {

	ArrayList<ArrayList<Vertex>> lattice;
	
	/**
	 * Constructor has to combine single source vertex from derivation with each phrase option. We'll
	 * be making a vertex for each coverage (for now, it's just size since we're monotone) and a hyperarc
	 * for all target phrases to the new vertex.
	 * @param l the lattice of src coverage to target phrases. [src-start][src-end][target phrases]
	 * @param derivation the initial derivation graph (nothing in it yet
	 */
	public PBCandidateGenerator(ArrayList<ArrayList<ArrayList<Phrase>>> l, Hypergraph derivation, Map<Hyperarc,Map<String,Object>> rules) {
		assert(derivation.sourceVertices.size() == 1);
		
		//all rules we create will have this start node as their tail
		Vertex v = derivation.sourceVertices.iterator().next();
		ArrayList<Vertex> tail = new ArrayList<Vertex>();
		tail.add(v);
		
		lattice = new ArrayList<ArrayList<Vertex>>();
		//we need to track all of these by start and coverage as vertices.
		//we also need to add things that start at 1 to the hypergraph(?)
		
		for (ArrayList<ArrayList<Phrase>> spanStart : l) {
			ArrayList<Vertex> latticeSpanStart = new ArrayList<Vertex>();
			for(ArrayList<Phrase> spanEnd : spanStart) {
				Vertex passive = new Vertex(new ArrayList<Hyperarc>());
				for(Phrase trans : spanEnd) {
					//make the arc we'll associate with this rule
					Hyperarc phraseRule = new Hyperarc(tail);
					
					//add arc info to hash
					HashMap<String,Object> ruleHash = new HashMap<String,Object>();
					ruleHash.put("TRANSLATION", trans);
					rules.put(phraseRule, ruleHash);
					
					//add to our vertex
					passive.addArc(phraseRule);
				}
				latticeSpanStart.add(passive);
				derivation.addNode(passive);
			}
			lattice.add(latticeSpanStart);
		}
	}

	
	public Collection<Vertex> retrieveCombinableNodesForActiveVertex(
			Vertex activeVertex, Map<String, Object> vertexState,
			Phrase sentence) {
		Coverage c = (Coverage)vertexState.get("COVERAGE");
		
		ArrayList<Vertex> transOpts = new ArrayList<Vertex>();
		int gapStart = c.nextGap(0);
		while (gapStart != -1) {
			int gapEnd = c.nextCover(gapStart);
			for (int e = gapStart; e < gapEnd && e < sentence.size(); e++) {
				Vertex opts = lattice.get(gapStart).get(e-gapStart);
				transOpts.add(opts);
			}
			gapStart = c.nextGap(gapEnd);
		}
		
		return transOpts;
	}
	
	public static void main(String[] args) {
		
	}

}
