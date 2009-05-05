package org.statmt.ghgd;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.PriorityQueue;

// this is a vertex
public class Or implements Comparable<Or>, Iterable<And> {

	ArrayList<And> alts = new ArrayList<And>();
	Weight myWeight;
	PriorityQueue<And> candidates = null;
	
	public Or(ArrayList<And> hyperarcsIn, Weight estWeightIn) {
		alts = hyperarcsIn;
		myWeight = estWeightIn;
	}
	
	public int compareTo(Or o) {
		return myWeight.compareTo(o.myWeight);
	}
	
	public void getCandidates() {
		candidates = new PriorityQueue<And>(alts);
	}

	public Iterator<And> iterator() {
		return alts.iterator();
	}

}
