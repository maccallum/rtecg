#ifndef __PEAKS_H__
#define __PEAKS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtecg.h"

/*
typedef struct _rtecg_peak_stats
{
	rtecg_float n;
	rtecg_float mean;
	rtecg_float sum;
	rtecg_float var;
	rtecg_ctr last_peak_index;
	rtecg_float measure; // for different peak detection algorithms, this will be a measure of the confidence or strength of the peak
} rtecg_peak_stats;
*/

rtecg_ctr rtecg_peak0(rtecg_float *buf, rtecg_ctr bufpos_r, rtecg_ctr buflen, int kneighbors);

//rtecg_peak_stats rtecg_peak1_s1(rtecg_peak_stats s, rtecg_float *buf, rtecg_ctr bufpos_r, rtecg_ctr buflen, int kneighbors, rtecg_float hstdevs);
//rtecg_ctr rtecg_peak_last_index(rtecg_peak_stats s);

#ifdef __cplusplus
}
#endif

#endif
