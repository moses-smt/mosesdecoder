package org.statmt.hg4;

public abstract class HyperarcVisitor {

	/**
	 * 
	 * @param h - the arc in question
	 * @param children - the results of applying transition to children
	 * @return application of the rule/value/etc associated with h to children
	 */
	public abstract Object transition(Hyperarc h, Object[] children);	    
	 
}
