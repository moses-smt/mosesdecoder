package org.statmt.hg4;

import java.util.BitSet;

public class Coverage implements Comparable {
	BitSet coverage;
	
	/**
	 * @param phraseSize
	 * @param coverageStart
	 * @param coverageEnd (exclusive)
	 */
	public Coverage(int phraseSize, int coverageStart, int coverageEnd) {
		coverage = new BitSet(phraseSize);
		coverage.set(coverageStart,coverageEnd,true);
	}

	/**
	 * Takes 'or' of params, asserts compatibility.
	 * @param a
	 * @param b
	 */
	public Coverage(Coverage a, Coverage b) {
		assert(a.isCompatible(b));
		this.coverage = (BitSet)a.coverage.clone();
		this.coverage.or(b.coverage);	
	}
	
	public int size() {
		return coverage.size();
	}
	
	public int nextGap(int fromIndex) {
		return coverage.nextClearBit(fromIndex);		
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
		assert(this.coverage.size() == other.coverage.size());
		return !this.coverage.intersects(other.coverage);
	}
	
	public int compareTo(Object o) {
        Coverage other = (Coverage) o;
		return other.coverage.cardinality() - this.coverage.cardinality();
	}
}
