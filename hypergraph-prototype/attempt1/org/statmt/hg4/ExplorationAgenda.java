package org.statmt.hg4;

import java.util.LinkedList;

public class ExplorationAgenda extends LinkedList<APPair> {

	private static final long serialVersionUID = 1L;

	/**
	 * Gets the next active vertex that should be processed. This vertex
	 * must be removed from the agenda!
	 * @return
	 */
	public APPair nextVertex() {
		return this.remove();
	}
		
	/**
	 * Add a vertex to the agenda.
	 */
	public void addPair(Vertex a, Vertex p) {
		this.add(new APPair(a, p));
	}
}
