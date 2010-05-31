package org.statmt.hg4;

import java.util.HashMap;

public class VertexState extends HashMap<String, Comparable> {

	private static final long serialVersionUID = 421421L;
	@Override
	public int hashCode() {
		int hc = 0;
		for (String x : this.keySet()) {
			hc *= 37;
			hc += x.hashCode();
		}
		for (Object o : this.values()) {
			hc *= 37;
			hc += o.hashCode();
		}
		return hc;
	}
	
	@Override
	public boolean equals(Object o) {
		VertexSignature v = (VertexSignature)o;
		int cs = v.size();
		for (String x : this.keySet()) {
			Object ot = this.get(x);
			if (!ot.equals(v.get(x)))
				return false;
			cs--;
		}
		return cs == 0;
	}

}
