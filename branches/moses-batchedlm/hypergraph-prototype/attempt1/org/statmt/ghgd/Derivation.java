package org.statmt.ghgd;

public class Derivation implements Comparable<Derivation> {

	And hyperArc;
	int[] positions;
	public Derivation(And hyperArc, int[] positions) {
		this.hyperArc = hyperArc;
		this.positions = positions;
	}
	public int compareTo(Derivation o) {
		return 0;
	}
	
}
