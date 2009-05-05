package org.statmt.hg4;

import java.util.ArrayList;
import java.util.List;

public class APPair {

	public Vertex active;
	public Vertex passive;
	
	public APPair(Vertex a, Vertex p) {
		active = a;
		passive = p;
	}
	
	public List<Vertex> asTail() {
		List<Vertex> res = new ArrayList<Vertex>(2);
		res.add(active);
		res.add(passive);
		return res;
	}
}
