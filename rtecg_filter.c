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
	s.xm24 = RTECG_LPBUFLEN / 2;
	return s;
}

rtecg_ptlp rtecg_ptlp_hx0(rtecg_ptlp s, rtecg_int x0)
{
	s.y0 = (s.y1 << 1) - s.y2 + x0 - (s.xs[s.xm24] << 1) + s.xs[s.xm48];
	s.y2 = s.y1;
	s.y1 = s.y0;
	s.xs[s.xm48] = x0;
	if(++(s.xm48) == RTECG_LPBUFLEN){
		s.xm48 = 0;
	}
	if(++(s.xm24) == RTECG_LPBUFLEN){
		s.xm24 = 0;
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
	s.xm64 = (RTECG_HPBUFLEN + 1) / 2;
	return s;
}

rtecg_pthp rtecg_pthp_hx0(rtecg_pthp s, rtecg_int x0)
{
	s.y += x0 - s.xs[s.xm128];
	s.z = s.xs[s.xm64] - (s.y / RTECG_HPBUFLEN);
	s.xs[s.xm128] = x0;
	if(++(s.xm128) == RTECG_HPBUFLEN){
		s.xm128 = 0;
	}
	if(++(s.xm64) == RTECG_HPBUFLEN){
		s.xm64 = 0;
	}
	return s;
}

rtecg_int rtecg_pthp_y0(rtecg_pthp s)
{
	return s.z;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// derivative approximation
////////////////////////////////////////////////////////////////////////////////////////////////////

rtecg_ptd rtecg_ptd_init(void)
{
	rtecg_ptd s;
	memset(&s, 0, sizeof(rtecg_ptd));
	return s;
}

rtecg_ptd rtecg_ptd_hx0(rtecg_ptd s, rtecg_int x0)
{
	s.y = x0 - s.xs[s.xm1];
	s.xs[s.xm1] = x0;
	if(++(s.xm1) == RTECG_DERIVLEN){
		s.xm1 = 0;
	}
	return s;
}

rtecg_int rtecg_ptd_y0(rtecg_ptd s)
{
	return s.y * s.y;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// moving window integrator
////////////////////////////////////////////////////////////////////////////////////////////////////

rtecg_ptmwi rtecg_ptmwi_init(void)
{
	rtecg_ptmwi s;
	memset(&s, 0, sizeof(rtecg_ptmwi));
	return s;
}

rtecg_ptmwi rtecg_ptmwi_hx0(rtecg_ptmwi s, rtecg_int x0)
{
	s.sum += x0;
	s.sum -= s.xs[s.ptr];
	s.xs[s.ptr] = x0;
	if(++(s.ptr) == RTECG_MWILEN){
		s.ptr = 0;
	}
	return s;
}

rtecg_int rtecg_ptmwi_y0(rtecg_ptmwi s)
{
	return s.sum / RTECG_MWILEN;
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
