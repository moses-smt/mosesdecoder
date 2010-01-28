package org.statmt.hg3;

import java.util.List;

public class Vertex {
	
	List<HyperArc> incomingArcs = null;
	
	public void addArc(HyperArc arc) {
		incomingArcs.add(arc);
		
	}
	
	public boolean hasChildren() {
		return (!incomingArcs.isEmpty());	
	}
	
	/**
	 * Constructor for non-source Vertex
	 * @param incomingArcs
	 */
	public Vertex(List<HyperArc> incomingArcs) {
		this.incomingArcs = incomingArcs;
	}
	
	

}
