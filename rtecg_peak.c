#include <math.h>
#include "rtecg.h"
#include "rtecg_peak.h"

rtecg_ctr rtecg_peak0(rtecg_float *buf, rtecg_ctr bufpos_r, rtecg_ctr buflen, int kneighbors)
{
	rtecg_ctr xipk = bufpos_r, xi = bufpos_r - kneighbors - 1, ximk = xi - kneighbors;
	rtecg_float yi = buf[xi];
	for(int i = (ximk % buflen); i < (xipk % buflen); i++){
		if(buf[i] > yi){
			return 0;
		}
	}
	return 1;
}

/*
https://www.researchgate.net/publication/228853276_Simple_Algorithms_for_Peak_Detection_in_Time-Series
 */

static rtecg_float rtecg_s1(rtecg_float *buf, rtecg_ctr bufpos_r, rtecg_ctr buflen, int k)
{
	int64_t xi = ((int64_t)bufpos_r - k) - 1;
	int64_t ximk = xi - k;
	rtecg_float l = buf[xi % buflen];
	rtecg_float maxl = -1;
	rtecg_float maxr = -1;
	for(int i = 0; i < k; i++){
		rtecg_float r = buf[(ximk + i) % buflen];
		rtecg_float d = l - r;
		if(d > maxl){
			maxl = d;
		}else{
		}
		r = buf[(xi + i + 1) % buflen];
		d = l - r;
		if(d > maxr){
			maxr = d;
		}else{
		}
	}
	return (maxl + maxr) / 2.;
}

static rtecg_peak_stats rtecg_calc_stats(rtecg_peak_stats s, rtecg_float xi)
{
	s.n++;
	rtecg_float prev_mean = s.mean;
	s.mean = s.mean + (xi - s.mean) / s.n;
	s.sum = s.sum + (xi - s.mean) * (xi - prev_mean);
	s.var = s.sum / s.n;
	return s;
}

rtecg_peak_stats rtecg_peak1_s1(rtecg_peak_stats s, rtecg_float *buf, rtecg_ctr bufpos_r, rtecg_ctr buflen, int kneighbors, rtecg_float hstdevs)
{
	int64_t xi = ((int64_t)bufpos_r - kneighbors) - 1;
	int64_t ximk = xi - kneighbors;
	if(ximk <= 0){ // the 0th index can't be a peak because we don't know what's to the left of it
		return s;
	}

	rtecg_float ai = rtecg_s1(buf, bufpos_r, buflen, kneighbors);
	s = rtecg_calc_stats(s, ai);
        if(ai > 0 && ((ai - s.mean) > (hstdevs * sqrt(s.var)))){
		s.last_peak_index = bufpos_r;
		s.measure = ai;
		return s;
	}else{
		return s;
	}
}

rtecg_ctr rtecg_peak_last_index(rtecg_peak_stats s)
{
	return s.last_peak_index;
}
