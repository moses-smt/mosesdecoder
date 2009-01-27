package org.statmt.hg4;

import java.util.BitSet;

public class Coverage implements Comparable<Coverage> {
	BitSet coverage;
	int slen;
	
	public Coverage(int phraseSize) {
		coverage = new BitSet(phraseSize);
		slen = phraseSize;
	}
	/**
	 * @param phraseSize
	 * @param coverageStart
	 * @param coverageEnd (exclusive)
	 */
	public Coverage(int phraseSize, int coverageStart, int coverageEnd) {
		coverage = new BitSet(phraseSize);
		coverage.set(coverageStart,coverageEnd,true);
		slen = phraseSize;
		System.out.println("C(" + phraseSize + "," + coverageStart + "," + coverageEnd +")");
	}

	/**
	 * Takes 'or' of params, asserts compatibility.
	 * @param a
	 * @param b
	 */
	public Coverage(Coverage a, Coverage b) {
		assert(a.isCompatible(b));
		slen = a.slen;
		assert(a.slen == b.slen);
		this.coverage = (BitSet)a.coverage.clone();
		this.coverage.or(b.coverage);	
	}
	
	public int size() {
		return slen;
	}
	
	@Override
	public int hashCode() {
		return 37 * slen + this.toString().hashCode();
	}
	
	@Override
	public boolean equals(Object o) {
		Coverage c = (Coverage)o;
		return this.slen == c.slen && this.toString().equals(c.toString());
	}
	
	public int nextGap(int fromIndex) {
		int loc = coverage.nextClearBit(fromIndex);		
		if (loc >= slen) return -1;
		else return loc;
	}
	
	public int nextCover(int fromIndex) {
		return coverage.nextSetBit(fromIndex);
	}
	
	/**
	 * Checks if coverages don't overlap.
	 * @param other
	 * @return
	 */
	public boolean isCompatible(Coverage other) {
		assert(this.slen == other.slen);
		return !this.coverage.intersects(other.coverage);
	}
	
	public boolean isAllCovered() { 
		return coverage.cardinality() == slen;
	}
	
	public int compareTo(Coverage other) {
		return other.coverage.cardinality() - this.coverage.cardinality();
	}
	
	public String toString() { return coverage.toString() + " size="+this.size(); }
}
