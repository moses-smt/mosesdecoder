package org.statmt.hg3;

import java.util.List;

public class HyperArc {

	List<Vertex> tail;
	
	public List<Vertex> getTail() {
		return tail;
	}
	
	public HyperArc(List<Vertex> tail) {
		this.tail = tail;
	}
}
