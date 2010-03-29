package org.statmt.hg4;

import java.util.Comparator;
import java.util.PriorityQueue;

public class VertexGroup extends PriorityQueue<Vertex> {

	FinishingAgenda agenda;
	VertexSignature sig;
	public VertexGroup(FinishingAgenda f, Comparator<Vertex> comp, VertexSignature sig) {
		super(5, comp);
		System.out.println("CREATE VG: " + comp);
		agenda = f;
		this.sig = sig;
	}
	
	public boolean isActive() {
		return (Boolean)this.sig.get("ACTIVE?");
	}
	
	@Override
	public boolean add(Vertex v) {
		super.add(v);
		if (super.peek() == v)
			agenda.notifyBest(this, v);
		return true;
	}
	
	public VertexSignature getSignature() {
		return sig;
	}
}
