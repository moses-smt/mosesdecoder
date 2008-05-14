package org.statmt.ghgd;

import java.util.ArrayList;

public class And implements Comparable<And> {

	Weight myWeight = null;
	Axiom axiom = null;
	ArrayList<Or> children = null;
	
	public And(Axiom a, ArrayList<Or> ants, Weight w) {
		axiom = a;
		children = ants;
		myWeight = w;
	}
	
	public int compareTo(And o) {
		return myWeight.compareTo(o.myWeight);
	}
	
	public Weight getWeight() { return myWeight; }
	
	public boolean isAxiom() { return children == null; }
	
	public ArrayList<Or> getChildren() { return children; }

}
