#include "rtecg.h"
#include "rtecg_filter.h"
#include <math.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// low pass filter
////////////////////////////////////////////////////////////////////////////////////////////////////

rtecg_ptlp rtecg_ptlp_init(void)
{
	rtecg_ptlp s;
	memset(&s, 0, sizeof(rtecg_ptlp));
	s.xm6 = 6;
	return s;
}

rtecg_ptlp rtecg_ptlp_hx0(rtecg_ptlp s, rtecg_int x0)
{
	// eq. 3
	// y(nT) = 2y(nT - T) - y(nT - 2T) + x(nT) - 2x(nT - 6T) + x(nT - 12T)
	s.y0 = (s.y1 << 1) - s.y2 + x0 - (s.xs[s.xm6] << 1) + s.xs[s.xm12];
	s.y2 = s.y1;
	s.y1 = s.y0;
	s.xs[s.xm12] = x0;
	if(++(s.xm12) == RTECG_LPBUFLEN){
		s.xm12 = 0;
	}
	if(++(s.xm6) == RTECG_LPBUFLEN){
		s.xm6 = 0;
	}
	return s;
}

rtecg_int rtecg_ptlp_y0(rtecg_ptlp s)
{
	return s.y0 / ((RTECG_LPBUFLEN * RTECG_LPBUFLEN) / 4);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// high pass filter
////////////////////////////////////////////////////////////////////////////////////////////////////

rtecg_pthp rtecg_pthp_init(void)
{
	rtecg_pthp s;
	memset(&s, 0, sizeof(rtecg_pthp));
	s.xm16 = 16;
	s.xm17 = 15;
	return s;
}

rtecg_pthp rtecg_pthp_hx0(rtecg_pthp s, rtecg_int x0)
{
	// eq. 6 from the original paper:
	// y(nT) = 32x(nT - 16T) - [y(nT - T) + x(nT) - x(nT - 32T)]
	// eq. 2.4 from the errata:
	// y(nT) = y(nT - T) - x(nT)/32 + x(nT - 16T) - x(nT - 17T) + x(nT - 32T)/32
	//s.y += x0 - s.xs[s.xm32];
	//s.z = s.xs[s.xm16] - (s.y / RTECG_HPBUFLEN);
	s.y = s.y - (x0 / 32) + s.xs[s.xm16] - s.xs[s.xm17] + (s.xs[s.xm32] / 32);
	s.xs[s.xm32] = x0;
	if(++(s.xm32) == RTECG_HPBUFLEN){
		s.xm32 = 0;
	}
	if(++(s.xm17) == RTECG_HPBUFLEN){
		s.xm17 = 0;
	}
	if(++(s.xm16) == RTECG_HPBUFLEN){
		s.xm16 = 0;
	}
	return s;
}

rtecg_int rtecg_pthp_y0(rtecg_pthp s)
{
	return s.y;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// derivative approximation
////////////////////////////////////////////////////////////////////////////////////////////////////

rtecg_ptd rtecg_ptd_init(void)
{
	rtecg_ptd s;
	memset(&s, 0, sizeof(rtecg_ptd));
	s.xm3 = 1;
	s.xm2 = 2;
	s.xm1 = 3;
	return s;
}

rtecg_ptd rtecg_ptd_hx0(rtecg_ptd s, rtecg_int x0)
{
	// eq. 2.6 from the errata:
	// y(nT) = (2x(nT) + x(nT - T) - x(nT - 3T) - 2x(nT - 4T)) / 8
	s.y = ((2 * x0) + s.xs[s.xm1] - s.xs[s.xm3] - (2 * s.xs[s.xm4])) / 8;
	s.xs[s.xm4] = x0;
	if(++(s.xm4) == RTECG_DERIVLEN){
		s.xm4 = 0;
	}
	if(++(s.xm3) == RTECG_DERIVLEN){
		s.xm3 = 0;
	}
	if(++(s.xm2) == RTECG_DERIVLEN){
		s.xm2 = 0;
	}
	if(++(s.xm1) == RTECG_DERIVLEN){
		s.xm1 = 0;
	}
	return s;
}

rtecg_int rtecg_ptd_y0(rtecg_ptd s)
{
	// the squaring function in eq. 10 in the original paper is included here
	return s.y * s.y;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// moving window integrator
////////////////////////////////////////////////////////////////////////////////////////////////////

rtecg_pti rtecg_pti_init(void)
{
	rtecg_pti s;
	memset(&s, 0, sizeof(rtecg_pti));
	return s;
}

rtecg_pti rtecg_pti_hx0(rtecg_pti s, rtecg_int x0)
{
	// eq. 11:
	// y(nT) = (1/N)[x(nT - (N-1)T) + x(nT - (N-2)T) + ... + x(nT)]
	s.sum += x0;
	s.sum -= s.xs[s.ptr];
	s.xs[s.ptr] = x0;
	if(++(s.ptr) == RTECG_MWILEN){
		s.ptr = 0;
	}
	return s;
}

rtecg_int rtecg_pti_y0(rtecg_pti s)
{
	return s.sum / RTECG_MWILEN;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// identify peaks
////////////////////////////////////////////////////////////////////////////////////////////////////

rtecg_pk rtecg_pk_init(void)
{
	rtecg_pk s;
	memset(&s, 0, sizeof(rtecg_pk));
	s.xmNm1d2 = RTECG_PKKNEIGH;
	return s;
}

// "A peak is a local maximum determined by observing when the
// signal changes direction within a predefined time interval." p. 233
// We keep a buffer of an odd number of samples, and determine whether
// the value at the midpoint of the buffer is the maximum (a peak) or not.
// As we check, we compute the maximum slope.
rtecg_pk rtecg_pk_mark(rtecg_pk s, rtecg_int x0)
{
	rtecg_int xmNm1d2 = s.xs[s.xmNm1d2];
	rtecg_int maxslope = 0; // only interested in positive slopes
	s.y0 = 1;
	rtecg_int ptr = s.xmNm1;
	rtecg_int last;
	if(ptr == 0){
		last = s.xs[RTECG_PKWINLEN - 1];
	}else{
		last = s.xs[ptr - 1];
	}
	for(int i = 0; i < RTECG_PKWINLEN; i++){
		if(s.xs[i] > xmNm1d2){
			s.y0 = 0;
			maxslope = 0;
			break;
		}
		if(i > 0){
			rtecg_int slope = s.xs[ptr] - last;
			if(slope > maxslope){
				maxslope = slope;
			}
		}
		last = s.xs[ptr];
		if(++(ptr) == RTECG_PKWINLEN){
			ptr = 0;
		}
	}
	s.xs[s.xmNm1] = x0;
	s.maxslope = maxslope;
	if(++(s.xmNm1) == RTECG_PKWINLEN){
		s.xmNm1 = 0;
	}
	if(++(s.xmNm1d2) == RTECG_PKWINLEN){
		s.xmNm1d2 = 0;
	}
	return s;
}

rtecg_int rtecg_pk_y0(rtecg_pk s)
{
	return s.y0;
}

rtecg_int rtecg_pk_xmNm1d2(rtecg_pk s)
{
	if(rtecg_pk_y0(s)){
		if(s.xmNm1d2 == 0){
			return s.xs[RTECG_PKWINLEN - 1];
		}else{
			return s.xs[s.xmNm1d2 - 1];
		}
	}else{
		return 0;
	}
}

rtecg_int rtecg_pk_maxslope(rtecg_pk s)
{
	return s.maxslope;
}

/*
rtecg_bq rtecg_bq_init(rtecg_float gain, rtecg_float a1, rtecg_float a2, rtecg_float b0, rtecg_float b1, rtecg_float b2)
{
	return (rtecg_bq){{0., 0., 0.}, {0., 0., 0.}, a1, a2, b0, b1, b2, gain};
}

rtecg_bq rtecg_bq_hx0(rtecg_bq s, rtecg_float x0)
{
	s.xs[0] = s.gain * x0;
	s.ys[0] = s.b0 * s.xs[0] + s.b1 * s.xs[1] + s.b2 * s.xs[2];
	s.ys[0] -= s.a2 * s.ys[2] + s.a1 * s.ys[1];
	s.xs[2] = s.xs[1];
	s.xs[1] = s.xs[0];
	s.ys[2] = s.ys[1];
	s.ys[1] = s.ys[0];
	return s;
}

rtecg_float rtecg_bq_y0(rtecg_bq s)
{
	return s.ys[0];
}

rtecg_bw rtecg_bw_init(int type, int order, rtecg_float fc, rtecg_float fs, rtecg_float gain)
{
	rtecg_bw s;
	memset(&s, 0, sizeof(rtecg_bw));
	if(order > 48){
		return s;
	}
	
	int pairs = order >> 1; 
	int oddpoles = order & 1;
	s.n = pairs + oddpoles;
	rtecg_float poleinc = M_PI / order;
	rtecg_float firstangle = poleinc;

	rtecg_float k = tan(M_PI * fc / fs);
	if(oddpoles == 0){
		firstangle /= 2.;
	}else{
		rtecg_float norm = 1. / (1. + k / 0.5 + k * k);
		s.bq[0] = rtecg_bq_init(gain, 2. * (k * k - 1) * norm, 0, k / 0.5 * norm, 0, 0);
	}
	for(int i = 0 + oddpoles; i < s.n; i++){
		rtecg_float q = 1. / (2. * cos(firstangle + (i - oddpoles) * poleinc));
		rtecg_float norm = 1. / (1. + k / q + k * k);
		s.bq[i] = rtecg_bq_init(gain, 2. * (k * k - 1) * norm, (1. - k / q + k * k) * norm, k / q * norm, 0.0, -1 * (k / q * norm));
	}
	return s;
}
	
rtecg_bw rtecg_bw_hx0(rtecg_bw s, rtecg_float x0)
{
	s.bq[0] = rtecg_bq_hx0(s.bq[0], x0);
	for(int i = 1; i < s.n; i++){
		s.bq[i] = rtecg_bq_hx0(s.bq[i], rtecg_bq_y0(s.bq[i - 1]));
	}
	return s;
}

rtecg_float rtecg_bw_y0(rtecg_bw s)
{
	return rtecg_bq_y0(s.bq[s.n - 1]);
}
*/
