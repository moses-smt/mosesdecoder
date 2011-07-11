package org.statmt.ghgd;

public class FloatArrayWeight extends Weight {

	float[] v;
	
	public FloatArrayWeight(int n) { v = new float[n]; }
	public FloatArrayWeight(float[] f) { v = f; }
	@Override
	public Weight add(Weight x) {
		FloatArrayWeight w=(FloatArrayWeight)x;
		assert(w.v.length == this.v.length);
		float[] r = new float[v.length];
		for (int i =0; i<v.length; i++) {
			r[i] = v[i] + w.v[i];
		}
		return new FloatArrayWeight(r);
	}
	
	public float innerProduct(FloatArrayWeight o) {
		FloatArrayWeight w = (FloatArrayWeight)o;
		float res = 0.0f;
		for (int i = 0; i<v.length; i++) {
			res += v[i] * w.v[i];
		}
		return res;
	}

	public int compareTo(Weight o) {
		FloatArrayWeight w = (FloatArrayWeight)o;
		float sum1 =0.0f;
		float sum2 =0.0f;
		for (int i = 0; i<v.length; i++) {
			sum1 += w.v[i];
		}
		for (int i = 0; i<v.length; i++) {
			sum2 += v[i];
		}
		float d = sum1 - sum2;
		if (d != 0.0f) 
			if (d < 0.0f) return -1; else return 1;
		return 0;
	}

}
