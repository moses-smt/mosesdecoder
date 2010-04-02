package org.statmt.hg4;

import java.util.Arrays;

public class Phrase implements Comparable<Phrase> {
   	String[] d;
	public Phrase(String ds) {
		d = ds.split("\\s+");
	}
	public final int size() { return d.length; }
	protected Phrase(String[] words) { d = words; }
	@Override
	public int hashCode() {
		int hc = 17;
		for (String x : d) {
			hc *= 37;
			hc += x.hashCode();
		}
		return hc; } 
	@Override
	public boolean equals(Object r) {
		Phrase p = (Phrase)r;
		if (p.d.length != this.d.length) return false;
		for (int i =0; i<d.length; i++)
			if (d[i].compareTo(p.d[i]) != 0) return false;
		return true;
	}
	public Phrase slice(int x, int y) {
		String[] w = new String[y - x];
		for (int i=x; i<y; i++)
			w[i-x] = d[i];
		return new Phrase(w);
	}
	public String toString() {
		String r= "<";
		for (int i=0; i<d.length; i++) if (i==0) r+=d[i]; else r += " " + d[i];
		r += ">";
		return r;
	}
	public int compareTo(Phrase o) {
		return this.toString().compareTo(o.toString());
	}

}
