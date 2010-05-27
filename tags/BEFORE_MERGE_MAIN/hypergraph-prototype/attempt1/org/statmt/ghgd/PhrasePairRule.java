package org.statmt.ghgd;

import java.util.ArrayList;
import java.util.Properties;

public class PhrasePairRule extends Rule {

	String e;
	String f;
	Weight w;
	
	public PhrasePairRule(String f, String e, Weight w) {
		this.f = f;
		this.e = e;
		this.w = w;
	}
	
	@Override
	public String recoverString(ArrayList<Or> children, Properties p) {
		if (children == null) return e;
		assert(children.size() == 1);
		Or prevHypo = children.get(0);
//		And curBestArc = prevHypo.alts.get(d.positions[0]);
		return null;
	}
	
	public String toString() {
		return "<PPA " + f + " ||| " + e + " ||| " + w + ">"; 
	}
}
