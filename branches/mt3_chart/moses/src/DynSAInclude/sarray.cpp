/*
Hybrid suffix-array builder, written by Sean Quinlan and Sean Doward,
distributed under the Plan 9 license, which reads in part

3.3 With respect to Your distribution of Licensed Software (or any
portion thereof), You must include the following information in a
conspicuous location governing such distribution (e.g., a separate
file) and on all copies of any Source Code version of Licensed
Software You distribute:

    "The contents herein includes software initially developed by
    Lucent Technologies Inc. and others, and is subject to the terms
    of the Lucent Technologies Inc. Plan 9 Open Source License
    Agreement.  A copy of the Plan 9 Open Source License Agreement is
    available at: http://plan9.bell-labs.com/plan9dist/download.html
    or by contacting Lucent Technologies at http: //www.lucent.com.
    All software distributed under such Agreement is distributed on,
    obligations and limitations under such Agreement.  Portions of
    the software developed by Lucent Technologies Inc. and others are
    Copyright (c) 2002.  All rights reserved.
    Contributor(s):___________________________"
*/
/*	
	int sarray(int a[], int n)
Purpose
	Return in a[] a suffix array for the original
	contents of a[].  (The original values in a[]
	are typically serial numbers of distinct tokens
	in some list.)

Precondition
	Array a[] holds n values, with n>=1.  Exactly k 
	distinct values, in the range 0..k-1, are present.
	Value 0, an endmark, appears exactly once, at a[n-1].

Postcondition
	Array a[] is a copy of the internal array p[]
	that records the sorting permutation: if i<j
	then the original suffix a[p[i]..n-1] is
	lexicographically less than a[p[j]..n-1].

Return value
	-1 on error.
	Otherwise index i such that a[i]==0, i.e. the
        index of the whole-string suffix, used in
	Burrows-Wheeler data compression.
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "sarray.h"
#include "vocab.h"

namespace Moses {

#define pred(i, h) ((t=(i)-(h))<0?  t+n: t)
#define succ(i, h) ((t=(i)+(h))>=n? t-n: t)

enum
{
	BUCK = ~(~0u>>1),	/* high bit */
	MAXI = ~0u>>1,		/* biggest int */
};

static	void	qsort2(int*, int*, int n);
static	int	ssortit(int a[], int p[], int n, int h, int *pe, int nbuck);

int
sarray(int a[], int n)
{
	int i, l;
	int c, cc, ncc, lab, cum, nbuck;
	int k;
	int *p = 0;
	int result = -1;
	int *al;
	int *pl;

	for(k=0,i=0; i<n; i++)	
		if(a[i] > k)
			k = a[i];	/* max element */
	k++;
	//if(k>n) 
		//goto out;

	nbuck = 0;
	p = (int *)malloc(n*sizeof(int));
	if(p == 0)
		goto out;

	pl = p + n - k;
	al = a;
	memset(pl, -1, k*sizeof(int));

	for(i=0; i<n; i++) {		/* (1) link */
		l = a[i];
		al[i] = pl[l];
		pl[l] = i;
	}

	//for(i=0; i<k; i++)		/* check input - no holes */
	//	if(pl[i]<0)             // this requires that the input be serial from 0 up without skipping any entries 
	//		goto out;

	
	lab = 0;			/* (2) create p and label a */
	cum = 0;
	i = 0;
	for(c = 0; c < k; c++){	
		for(cc = pl[c]; cc != -1; cc = ncc){
			ncc = al[cc];
			al[cc] = lab;
			cum++;
			p[i++] = cc;
		}
		if(lab + 1 == cum) {
			i--;
		} else {
			p[i-1] |= BUCK;
			nbuck++;
		}
		lab = cum;
	}
	result = ssortit(a, p, n, 1, p+i, nbuck);
	memcpy(a, p, n*sizeof(int));
	
out:
	free(p);
	return result;
}

/* bsarray(uchar buf[], int p[], int n)
 * The input, buf, is an arbitrary byte array of length n.
 * The input is copied to temporary storage, relabeling 
 * pairs of input characters and appending a unique end marker 
 * having a value that is effectively less than any input byte.
 * The suffix array of this extended input is computed and
 * stored in p, which must have length at least n+1.
 *
 * Returns the index of the identity permutation (regarding
 * the suffix array as a list of circular shifts),
 * or -1 if there was an error.
 */
/*
  int
bsarray(const uchar buf[], int p[], int n)
{
	int *a, buckets[256*256];
	int i, last, cum, c, cc, ncc, lab, id, nbuck;

	a = malloc((n+1)*sizeof(int));
	if(a == 0)
		return -1;


	memset(buckets, -1, sizeof(buckets));
	c = buf[n-1] << 8;
	last = c;
	for(i = n - 2; i >= 0; i--){
		c = (buf[i] << 8) | (c >> 8);
		a[i] = buckets[c];
		buckets[c] = i;
	}

	/*
	 * end of string comes before anything else
	 */
/*
	a[n] = 0;

	lab = 1;
	cum = 1;
	i = 0;
	nbuck = 0;
	for(c = 0; c < 256*256; c++) {
		/*
		 * last character is followed by unique end of string
		 */
/*
		if(c == last) {
			a[n-1] = lab;
			cum++;
			lab++;
		}

		for(cc = buckets[c]; cc != -1; cc = ncc) {
			ncc = a[cc];
			a[cc] = lab;
			cum++;
			p[i++] = cc;
		}
		if(lab == cum)
			continue;
		if(lab + 1 == cum)
			i--;
		else {
			p[i - 1] |= BUCK;
			nbuck++;
		}
		lab = cum;
	}

	id = ssortit(a, p, n+1, 2, p+i, nbuck);
	free(a);
	return id;
}
*/
static int
ssortit(int a[], int p[], int n, int h, int *pe, int nbuck)
{
	int *s, *ss, *packing, *sorting;
	int v, sv, vv, packed, lab, t, i;

	for(; h < n && p < pe; h=2*h) {
		packing = p;
		nbuck = 0;

		for(sorting = p; sorting < pe; sorting = s){
			/*
			 * find length of stuff to sort
			 */
			lab = a[*sorting];
			for(s = sorting; ; s++) {
				sv = *s;
				v = a[succ(sv & ~BUCK, h)];
				if(v & BUCK)
					v = lab;
				a[sv & ~BUCK] = v | BUCK;
				if(sv & BUCK)
					break;
			}
			*s++ &= ~BUCK;
			nbuck++;

			qsort2(sorting, a, s - sorting);

			v = a[*sorting];
			a[*sorting] = lab;
			packed = 0;
			for(ss = sorting + 1; ss < s; ss++) {
				sv = *ss;
				vv = a[sv];
				if(vv == v) {
					*packing++ = ss[-1];
					packed++;
				} else {
					if(packed) {
						*packing++ = ss[-1] | BUCK;
					}
					lab += packed + 1;
					packed = 0;
					v = vv;
				}
				a[sv] = lab;
			}
			if(packed) {
				*packing++ = ss[-1] | BUCK;
			}
		}
		pe = packing;
	}

	/*
	 * reconstuct the permutation matrix
	 * return index of the entire string
	 */
	v = a[0];
	for(i = 0; i < n; i++)
		p[a[i]] = i;

	return v;
}

/*
 * qsort from Bentley and McIlroy, Software--Practice and Experience
   23 (1993) 1249-1265, specialized for sorting permutations based on
   successors
 */
static void
vecswap2(int *a, int *b, int n)
{
	while (n-- > 0) {
        	int t = *a;
		*a++ = *b;
		*b++ = t;
	}
}

#define swap2(a, b) { t = *(a); *(a) = *(b); *(b) = t; }

static int*
med3(int *a, int *b, int *c, int *asucc)
{
	int va, vb, vc;

	if ((va=asucc[*a]) == (vb=asucc[*b]))
		return a;
	if ((vc=asucc[*c]) == va || vc == vb)
		return c;	   
	return va < vb ?
		  (vb < vc ? b : (va < vc ? c : a))
		: (vb > vc ? b : (va < vc ? a : c));
}

static void
inssort(int *a, int *asucc, int n)
{
	int *pi, *pj, t;

	for (pi = a + 1; --n > 0; pi++)
		for (pj = pi; pj > a; pj--) {
			if(asucc[pj[-1]] <= asucc[*pj])
				break;
			swap2(pj, pj-1);
		}
}

static void
qsort2(int *a, int *asucc, int n)
{
	int d, r, partval;
	int *pa, *pb, *pc, *pd, *pl, *pm, *pn, t;

	if (n < 15) {
		inssort(a, asucc, n);
		return;
	}
	pl = a;
	pm = a + (n >> 1);
	pn = a + (n-1);
	if (n > 30) { /* On big arrays, pseudomedian of 9 */
		d = (n >> 3);
		pl = med3(pl, pl+d, pl+2*d, asucc);
		pm = med3(pm-d, pm, pm+d, asucc);
		pn = med3(pn-2*d, pn-d, pn, asucc);
	}
	pm = med3(pl, pm, pn, asucc);
	swap2(a, pm);
	partval = asucc[*a];
	pa = pb = a + 1;
	pc = pd = a + n-1;
	for (;;) {
		while (pb <= pc && (r = asucc[*pb]-partval) <= 0) {
			if (r == 0) {
				swap2(pa, pb);
				pa++;
			}
			pb++;
		}
		while (pb <= pc && (r = asucc[*pc]-partval) >= 0) {
			if (r == 0) {
				swap2(pc, pd);
				pd--;
			}
			pc--;
		}
		if (pb > pc)
			break;
		swap2(pb, pc);
		pb++;
		pc--;
	}
	pn = a + n;
	r = pa-a;
	if(pb-pa < r)
		r = pb-pa;
	vecswap2(a, pb-r, r);
	r = pn-pd-1;
	if(pd-pc < r)
		r = pd-pc;
	vecswap2(pb, pn-r, r);
	if ((r = pb-pa) > 1)
		qsort2(a, asucc, r);
	if ((r = pd-pc) > 1)
		qsort2(a + n-r, asucc, r);
}

} // end namespace
