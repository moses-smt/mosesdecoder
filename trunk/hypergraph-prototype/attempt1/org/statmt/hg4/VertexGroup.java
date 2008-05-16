package org.statmt.hg4;

import java.util.Comparator;
import java.util.Map;
import java.util.PriorityQueue;

public class VertexGroup extends PriorityQueue<Vertex> {

	FinishingAgenda agenda;
	Map<String, Object> sig;
	public VertexGroup(FinishingAgenda f, Comparator<Vertex> comp, Map<String, Object> sig) {
		super(5, comp);
		agenda = f;
		this.sig = sig;
	}
	
	@Override
	public boolean add(Vertex v) {
		super.add(v);
		if (this.peek() == v)
			agenda.notifyBest(this, v);
		return true;
	}
	
	public Map<String, Object> getSignature() {
		return sig;
	}
}
