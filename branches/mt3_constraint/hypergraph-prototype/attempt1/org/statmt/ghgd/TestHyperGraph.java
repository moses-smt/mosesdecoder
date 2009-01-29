package org.statmt.ghgd;

import java.util.ArrayList;

public class TestHyperGraph {

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		PhrasePairRule p1 = new PhrasePairRule("guten", "good", new FWeight(0.7f)); 
		PhrasePairRule p2 = new PhrasePairRule("guten", "well", new FWeight(0.3f)); 
		PhrasePairRule p3 = new PhrasePairRule("tag", "day", new FWeight(0.6f)); 
		PhrasePairRule p4 = new PhrasePairRule("tag", "hi", new FWeight(0.4f)); 
		PhrasePairRule p5 = new PhrasePairRule("guten tag", "hello", new FWeight(0.2f)); 
		PhrasePairRule p6 = new PhrasePairRule("guten tag", "good day", new FWeight(0.8f));
		And d1 = new And(p1, new ArrayList(), p1.w);
		And d2 = new And(p2, new ArrayList(), p2.w);
		ArrayList<And> a1 = new ArrayList<And>(2);
		a1.add(d1); a1.add(d2);
		Weight w1 = d1.getWeight();
		Weight w2 = d2.getWeight();
		Weight best = null;
		if (w1.compareTo(w2) < 0) best = w2; else best = w1;
		Or c1 = new Or(a1, best);

		ArrayList<Or> ants1 = new ArrayList<Or>(1); ants1.add(c1);
		And d3 = new And(p3, ants1, p3.w);
		And d4 = new And(p4, ants1, p4.w);
		ArrayList<And> a2 = new ArrayList<And>(2);
		a2.add(d3); a2.add(d4);
		w1 = d3.getWeight();
		w2 = d4.getWeight();
		best = null;
		if (w1.compareTo(w2) < 0) best = w2; else best = w1;
		Or c2 = new Or(a2, best);

		And d5 = new And(p5, new ArrayList(), p5.w);
		And d6 = new And(p6, new ArrayList(), p6.w);
		ArrayList<And> a3 = new ArrayList<And>(2);
		a3.add(d5); a3.add(d6);
		w1 = d5.getWeight();
		w2 = d6.getWeight();
		best = null;
		if (w1.compareTo(w2) < 0) best = w2; else best = w1;
		Or c3 = new Or(a3, best);
				
	}

}
