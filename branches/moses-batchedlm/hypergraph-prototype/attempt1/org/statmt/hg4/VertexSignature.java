package org.statmt.hg4;

import java.util.HashMap;

public class VertexSignature extends HashMap<String,Comparable> implements Comparable<VertexSignature> {

	private static final long serialVersionUID = 13434L;

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

	public int compareTo(VertexSignature o) {
		VertexSignature v = (VertexSignature)o;
		int cs = v.size();
		for (String x : this.keySet()) {
			Comparable<Object> ot = this.get(x);
			if (ot.compareTo(v.get(x)) != 0)
				return ot.compareTo(v.get(x));
			cs--;
		}
		return cs;
	}
}
