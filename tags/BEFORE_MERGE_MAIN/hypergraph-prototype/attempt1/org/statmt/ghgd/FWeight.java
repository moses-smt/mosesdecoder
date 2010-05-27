package org.statmt.ghgd;

public class FWeight extends Weight {

	float f;
	
	public FWeight(float x) { f = x; }
	@Override
	public Weight add(Weight x) {
		return new FWeight(((FWeight)x).f + f);
	}

	public int compareTo(Weight o) {
		float d = f - ((FWeight)o).f;
		if (d != 0.0f) {
			if (d < 0.0f) return -1; else return 1;
		}
		return 0;
	}
	
	public String toString() { return Float.toString(f); }

}
