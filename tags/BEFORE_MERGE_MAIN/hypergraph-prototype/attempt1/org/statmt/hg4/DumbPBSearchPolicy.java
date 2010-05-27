package org.statmt.hg4;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Map;

public class DumbPBSearchPolicy extends SearchStrategy {

	Phrase sentence;
	ArrayList<ArrayList<ArrayList<Vertex>>> lattice;
	
	/**
	 * Constructor has to combine single source vertex from derivation with each phrase option. We'll
	 * be making a vertex for each coverage (for now, it's just size since we're monotone) and a hyperarc
	 * for all target phrases to the new vertex.
	 * @param l the lattice of src coverage to target phrases. [src-start][src-end][target phrases]
	 * @param derivation the initial derivation graph (nothing in it yet
	 */
	public DumbPBSearchPolicy(Phrase p, ArrayList<ArrayList<ArrayList<Phrase>>> l, Hypergraph derivation,
			Map<Vertex, VertexState> v2s) {
		assert(derivation.sourceVertices.size() == 1);
		sentence = p;
		
		//all rules we create will have this start node as their tail
		Vertex source = derivation.sourceVertices.iterator().next();
		
		lattice = new ArrayList<ArrayList<ArrayList<Vertex>>>();
		//we need to track all of these by start and coverage as vertices.
		//we also need to add things that start at 1 to the hypergraph(?)
		
		int i = 0;
		for (ArrayList<ArrayList<Phrase>> spanStart : l) {
			ArrayList<ArrayList<Vertex>> latticeSpanStart = new ArrayList<ArrayList<Vertex>>();
			int j = 0;
			for(ArrayList<Phrase> spanEnd : spanStart) {
				j++;
				ArrayList<Vertex> opts = new ArrayList<Vertex>();
				for(Phrase trans : spanEnd) {
					ArrayList<Vertex> tail = new ArrayList<Vertex>();
					tail.add(source);
					Vertex passive = new Vertex(new ArrayList<Hyperarc>());

					//make the arc we'll associate with this rule
					Hyperarc phraseRule = new Hyperarc(tail);
					
					//add arc info to hash
					VertexState vs = new VertexState();
					vs.put("E", trans);
					vs.put("COVERAGE", new Coverage(l.size(), i, j + i));
					vs.put("PROB", new Float(0.0f));
					vs.put("ACTIVE?", new Boolean(false));

					v2s.put(passive, vs);
					//add to our vertex
					passive.addArc(phraseRule);
					derivation.addNode(passive);
					
					opts.add(passive);
				}
				latticeSpanStart.add(opts);
			}
			i++;
			lattice.add(latticeSpanStart);
		}
	}

	@Override
	public VertexState applyFundamentalRule(Hyperarc ha,
			Map<Vertex, VertexState> v2s) {
		Vertex hypo = ha.getTail().get(0);
		Vertex ext  = ha.getTail().get(1);
		System.out.println("DEBUG: Extending " + hypo.hashCode() + " with " + v2s.get(ext));
		float p1= (Float)v2s.get(hypo).get("PROB");
		float p2=  (Float)v2s.get(ext).get("PROB");
		VertexState res = new VertexState();
		Coverage c1=(Coverage)v2s.get(hypo).get("COVERAGE");
		Coverage c2= (Coverage)v2s.get(ext).get("COVERAGE");
		Coverage rc = new Coverage(c1,c2);
		res.put("COVERAGE", rc);
		res.put("PROB",     new Float(p1+p2));
		Boolean vt = new Boolean(!rc.isAllCovered());
		res.put("ACTIVE?", vt);
		//System.out.println("res="+res);
		return res;
	}

	@Override
	public void generateTraversals(VertexGroup activeEdges,
			Collection<Vertex> compatibleKnownThings,
			ExplorationAgenda agenda) {
		for (Vertex hypo : activeEdges) {
			for (Vertex transOpt : compatibleKnownThings) {
				System.out.println(" GT: " + hypo + " " + transOpt);
				agenda.addPair(hypo, transOpt);
			}
		}
	}

	@Override
	public Collection<Vertex> retrieveCombinableVertices(
			VertexSignature signatureOfVertex,
			Hypergraph derivationGraph, Phrase sentence) {
		Coverage c = (Coverage)signatureOfVertex.get("COVERAGE");
		
		ArrayList<Vertex> transOpts = new ArrayList<Vertex>();
		int gapStart = c.nextGap(0);
		while (gapStart != -1) {
			int gapEnd = c.nextCover(gapStart);
			if (gapEnd == -1) gapEnd = sentence.size();
			for (int e = gapStart; e < gapEnd && e < sentence.size(); e++) {
				ArrayList<Vertex> opts = lattice.get(gapStart).get(e-gapStart);
				transOpts.addAll(opts);
			}
			gapStart = c.nextGap(gapEnd);
		}
		
		return transOpts;
	}

}
