package org.statmt.hg4;

import java.util.ArrayList;
import java.util.HashMap;

public class PTable extends HashMap<Phrase,ArrayList<Phrase>> {
	public ArrayList<ArrayList<ArrayList<Phrase>>> buildLattice(Phrase sentence) {
		ArrayList<ArrayList<ArrayList<Phrase>>> r = new ArrayList<ArrayList<ArrayList<Phrase>>>();
		
    		for (int i = 0; i < sentence.size(); i++) {
    			ArrayList<ArrayList<Phrase>> x = new ArrayList<ArrayList<Phrase>>();
    			r.add(x);
    			for (int j = i + 1; j <= sentence.size(); j++) {
    				ArrayList<Phrase> opts =
    					this.get(sentence.slice(i, j));
    				if (opts == null)
    					opts = new ArrayList<Phrase>();
    				x.add(opts);
    			}
    		}
    		return r;
    	}	
	public void add(Phrase f, Phrase e) {
		ArrayList<Phrase> x = this.get(f);
		if (x == null) { x=new ArrayList<Phrase>(); this.put(f, x); }
		x.add(e);
	}
}

