package org.statmt.hg4;

import java.util.BitSet;

public class Coverage implements Comparable {
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
	
	public int compareTo(Object o) {
        Coverage other = (Coverage) o;
		return other.coverage.cardinality() - this.coverage.cardinality();
	}
	
	public String toString() { return coverage.toString() + " size="+this.size(); }
}
