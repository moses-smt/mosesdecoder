package org.statmt.ghgd;

import java.util.ArrayList;

public class And implements Comparable<And> {

	Weight myWeight = null;
	Rule rule = null;
	ArrayList<Or> children = null;
	
	public And(Rule r, ArrayList<Or> ants, Weight w) {
		rule = r;
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
